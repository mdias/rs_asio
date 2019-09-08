#include "stdafx.h"
#include "AsioSharedHost.h"
#include "RSAsioDevice.h"
#include "RSAsioAudioClient.h"
#include "RSAsioAudioRenderClient.h"
#include "RSAsioAudioCaptureClient.h"

RSAsioAudioClient::RSAsioAudioClient(RSAsioDevice& asioDevice)
	: m_AsioDevice(asioDevice)
	, m_AsioSharedHost(m_AsioDevice.GetAsioHost())
{
	m_AsioDevice.AddRef();
	m_AsioSharedHost.AddRef();

	memset(&m_WaveFormat, 0, sizeof(m_WaveFormat));
}

RSAsioAudioClient::~RSAsioAudioClient()
{
	m_AsioSharedHost.RemoveBufferSwitchListener(this);

	m_AsioSharedHost.Release();
	m_AsioDevice.Release();
}

HRESULT RSAsioAudioClient::QueryInterface(REFIID riid, void **ppvObject)
{
	if (!ppvObject)
		return E_POINTER;

	if (riid == __uuidof(IAudioClient) || riid == __uuidof(IAudioClient2) || riid == __uuidof(IAudioClient3))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}

	return ComBaseUnknown<IAudioClient3>::QueryInterface(riid, ppvObject);
}

HRESULT RSAsioAudioClient::Initialize(AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid)
{
	if (!pFormat)
		return E_POINTER;

	if (m_IsInitialized)
		return AUDCLNT_E_ALREADY_INITIALIZED;

	const bool useEventCallback = StreamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	if (useEventCallback && hnsBufferDuration != hnsPeriodicity)
		return AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (FAILED(IsFormatSupported(ShareMode, pFormat, nullptr)))
		return E_FAIL;

	if (m_AsioSharedHost.Start(*pFormat, hnsBufferDuration) != ASE_OK)
		return E_FAIL;

	memset(&m_WaveFormat, 0, sizeof(m_WaveFormat));
	if (pFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		m_WaveFormat = *(WAVEFORMATEXTENSIBLE*)pFormat;
	}
	else
	{
		m_WaveFormat.Format = *pFormat;
	}

	UpdateChannelMap();

	if (m_AsioDevice.IsOutput())
	{
		m_RenderClient = new RSAsioAudioRenderClient(*this);
	}
	else
	{
		m_CaptureClient = new RSAsioAudioCaptureClient(*this);
	}
	m_AsioSharedHost.AddBufferSwitchListener(this);

	m_IsInitialized = true;
	m_UsingEventHandle = useEventCallback;

	UINT32 numFrames = m_AsioSharedHost.GetBufferNumFrames();

	{
		std::lock_guard<std::mutex> g(m_bufferMutex);
		m_bufferNumFrames = numFrames;
		m_buffer.resize(pFormat->nBlockAlign * numFrames);
	}

	return S_OK;
}

HRESULT RSAsioAudioClient::GetBufferSize(UINT32 *pNumBufferFrames)
{
	if (!pNumBufferFrames)
		return E_POINTER;

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	*pNumBufferFrames = m_AsioSharedHost.GetBufferNumFrames();

	return S_OK;
}

HRESULT RSAsioAudioClient::GetStreamLatency(REFERENCE_TIME *phnsLatency)
{
	if (!phnsLatency)
		return E_POINTER;

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	REFERENCE_TIME in, out;
	if (!m_AsioSharedHost.GetLatencyTime(in, out))
		return E_FAIL;

	*phnsLatency = m_AsioDevice.IsOutput() ? out : in;

	return S_OK;
}

HRESULT RSAsioAudioClient::GetCurrentPadding(UINT32 *pNumPaddingFrames)
{
	if (!pNumPaddingFrames)
		return E_POINTER;

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	bool isBufferWaiting = false;

	if (m_AsioDevice.IsOutput())
	{
		if (m_RenderClient)
			isBufferWaiting = m_RenderClient->HasNewBufferWaiting();
	}
	else
	{
		if (m_CaptureClient)
			isBufferWaiting = m_CaptureClient->HasNewBufferWaiting();
	}

	*pNumPaddingFrames = isBufferWaiting ? m_bufferNumFrames : 0;

	return S_OK;
}

HRESULT RSAsioAudioClient::IsFormatSupported(AUDCLNT_SHAREMODE ShareMode, const WAVEFORMATEX *pFormat, WAVEFORMATEX **ppClosestMatch)
{
	if (!pFormat)
		return E_POINTER;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	// check channels
	if (pFormat->nChannels > m_AsioDevice.GetNumWasapiChannels())
	{
		//std::cerr << "  unsupported number of channels: " << pFormat->nChannels << "\n";
		return E_FAIL;
	}
	if (m_AsioDevice.GetNumChannels() == 0)
	{
		return E_FAIL;
	}

	// check compatibility with asio host
	if (!m_AsioSharedHost.IsWaveFormatSupported(*pFormat, m_AsioDevice.IsOutput(), m_AsioDevice.GetBaseChannelNumber(), m_AsioDevice.GetNumChannels()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT RSAsioAudioClient::GetMixFormat(WAVEFORMATEX **ppDeviceFormat)
{
	std::cout << __FUNCTION__ "\n";

	if (!ppDeviceFormat)
		return E_POINTER;

	ASIOSampleRate sampleRate = 0.0;
	if (m_AsioSharedHost.GetDriver()->getSampleRate(&sampleRate) != ASE_OK)
		return E_FAIL;

	WAVEFORMATEX* fmt = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
	fmt->wFormatTag = WAVE_FORMAT_PCM;
	fmt->nChannels = m_AsioDevice.GetNumWasapiChannels();
	fmt->wBitsPerSample = 16;
	fmt->nSamplesPerSec = std::lround(sampleRate);
	fmt->nBlockAlign = fmt->nChannels * (fmt->wBitsPerSample / 8);
	fmt->nAvgBytesPerSec = fmt->nBlockAlign * fmt->nSamplesPerSec;
	fmt->cbSize = 0; // no extra info

	*ppDeviceFormat = fmt;

	return S_OK;
}

HRESULT RSAsioAudioClient::GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod, REFERENCE_TIME *phnsMinimumDevicePeriod)
{
	std::cout << __FUNCTION__ "\n";

	if (!phnsDefaultDevicePeriod && !phnsMinimumDevicePeriod)
		return E_POINTER;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (phnsDefaultDevicePeriod)
	{
		*phnsDefaultDevicePeriod = MilisecsToRefTime(3);;
	}
	if (phnsMinimumDevicePeriod)
	{
		*phnsMinimumDevicePeriod = MilisecsToRefTime(3);
	}

	return S_OK;
}

HRESULT RSAsioAudioClient::Start()
{
	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (m_UsingEventHandle && !m_EventHandle)
		return AUDCLNT_E_EVENTHANDLE_NOT_SET;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (m_IsStarted)
		return AUDCLNT_E_NOT_STOPPED;

	m_IsStarted = true;

	return S_OK;
}

HRESULT RSAsioAudioClient::Stop()
{
	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	m_IsStarted = false;

	return S_OK;
}

HRESULT RSAsioAudioClient::Reset()
{
	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;
	if (m_IsStarted)
		return AUDCLNT_E_NOT_STOPPED;

	m_AsioSharedHost.RemoveBufferSwitchListener(this);
	if (m_RenderClient)
	{
		m_RenderClient->Release();
		m_RenderClient = nullptr;
	}
	if (m_CaptureClient)
	{
		m_CaptureClient->Release();
		m_CaptureClient = nullptr;
	}

	m_AsioSharedHost.Stop();
	m_IsInitialized = false;
	m_EventHandle = NULL;
	memset(&m_WaveFormat, 0, sizeof(m_WaveFormat));

	{
		std::lock_guard<std::mutex> g(m_bufferMutex);
		m_bufferNumFrames = 0;
		m_buffer.clear();
	}

	return S_OK;
}

HRESULT RSAsioAudioClient::SetEventHandle(HANDLE eventHandle)
{
	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (!m_UsingEventHandle)
		return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;

	m_EventHandle = eventHandle;

	return S_OK;
}

HRESULT RSAsioAudioClient::GetService(REFIID riid, void **ppv)
{
	if (riid == __uuidof(IAudioRenderClient))
	{
		if (m_RenderClient)
		{
			std::cout << "  returning render client\n";
			m_RenderClient->AddRef();
			*ppv = m_RenderClient;
			return S_OK;
		}
	}
	else if (riid == __uuidof(IAudioCaptureClient))
	{
		if (m_CaptureClient)
		{
			std::cout << "  returning capture client\n";
			m_CaptureClient->AddRef();
			*ppv = m_CaptureClient;
			return S_OK;
		}
	}

	return E_NOINTERFACE;
}


HRESULT RSAsioAudioClient::IsOffloadCapable(AUDIO_STREAM_CATEGORY Category, BOOL *pbOffloadCapable)
{
	return E_NOTIMPL;
}

HRESULT RSAsioAudioClient::SetClientProperties(const AudioClientProperties *pProperties)
{
	if (!pProperties)
		return E_POINTER;

	return E_NOTIMPL;
}

HRESULT RSAsioAudioClient::GetBufferSizeLimits(const WAVEFORMATEX *pFormat, BOOL bEventDriven, REFERENCE_TIME *phnsMinBufferDuration, REFERENCE_TIME *phnsMaxBufferDuration)
{
	if (!pFormat || !phnsMinBufferDuration || !phnsMaxBufferDuration)
		return E_POINTER;

	return E_NOTIMPL;
}


HRESULT RSAsioAudioClient::GetSharedModeEnginePeriod(const WAVEFORMATEX *pFormat, UINT32 *pDefaultPeriodInFrames, UINT32 *pFundamentalPeriodInFrames, UINT32 *pMinPeriodInFrames, UINT32 *pMaxPeriodInFrames)
{
	if (!pFormat || !pDefaultPeriodInFrames || !pFundamentalPeriodInFrames || !pMinPeriodInFrames || !pMaxPeriodInFrames)
		return E_POINTER;

	return E_NOTIMPL;
}

HRESULT RSAsioAudioClient::GetCurrentSharedModeEnginePeriod(WAVEFORMATEX **ppFormat, UINT32 *pCurrentPeriodInFrames)
{
	if (!ppFormat || !pCurrentPeriodInFrames)
		return E_POINTER;

	return E_NOTIMPL;
}

HRESULT RSAsioAudioClient::InitializeSharedAudioStream(DWORD StreamFlags, UINT32 PeriodInFrames, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid)
{
	if (!pFormat)
		return E_POINTER;

	return E_NOTIMPL;
}

template<typename TSample>
static void CopyDeinterleaveChannel(BYTE* inInterleaved, BYTE* outDeinterleaved, WORD inputChannel, WORD inputFrameSize, DWORD numFrames)
{
	inInterleaved += sizeof(TSample) * inputChannel;

	for (DWORD frame = 0; frame < numFrames; ++frame)
	{
		*(TSample*)outDeinterleaved = *(TSample*)inInterleaved;
		inInterleaved += inputFrameSize;
		outDeinterleaved += sizeof(TSample);
	}
}

template<typename TSample>
static void CopyInterleaveChannel(BYTE* inDeinterleaved, BYTE* outInterleaved, WORD outputChannel, WORD outputFrameSize, DWORD numFrames)
{
	outInterleaved += sizeof(TSample) * outputChannel;

	for (DWORD frame = 0; frame < numFrames; ++frame)
	{
		*(TSample*)outInterleaved = *(TSample*)inDeinterleaved;
		inDeinterleaved += sizeof(TSample);
		outInterleaved += outputFrameSize;
	}
}


void RSAsioAudioClient::OnAsioBufferSwitch(unsigned buffIdx)
{
	std::lock_guard<std::mutex> g(m_bufferMutex);

	if (!m_IsStarted)
	{
		memset(m_buffer.data(), 0, m_buffer.size());
		return;
	}

	// sanity check
	if (m_ChannelMap.size() < m_WaveFormat.Format.nChannels)
		return;

	if (m_AsioDevice.IsOutput())
	{
		for (WORD ch = 0; ch < m_WaveFormat.Format.nChannels; ++ch)
		{
			const int asioCh = m_ChannelMap[ch];
			if (asioCh >= 0)
			{
				ASIOBufferInfo* bufferInfo = m_AsioSharedHost.GetOutputBuffer(asioCh);
				if (bufferInfo)
				{
					if (m_WaveFormat.Format.wBitsPerSample == 16)
						CopyDeinterleaveChannel<std::int16_t>(m_buffer.data(), (BYTE*)bufferInfo->buffers[buffIdx], ch, m_WaveFormat.Format.nBlockAlign, m_bufferNumFrames);
					else if (m_WaveFormat.Format.wBitsPerSample == 32)
						CopyDeinterleaveChannel<std::int32_t>(m_buffer.data(), (BYTE*)bufferInfo->buffers[buffIdx], ch, m_WaveFormat.Format.nBlockAlign, m_bufferNumFrames);
				}
			}
		}
	}
	else
	{
		for (WORD ch = 0; ch < m_WaveFormat.Format.nChannels; ++ch)
		{
			const int asioCh = m_ChannelMap[ch];
			if (asioCh >= 0)
			{
				ASIOBufferInfo* bufferInfo = m_AsioSharedHost.GetInputBuffer(asioCh);
				if (bufferInfo)
				{
					if (m_WaveFormat.Format.wBitsPerSample == 16)
						CopyInterleaveChannel<std::int16_t>((BYTE*)bufferInfo->buffers[buffIdx], m_buffer.data(), ch, m_WaveFormat.Format.nBlockAlign, m_bufferNumFrames);
					else if (m_WaveFormat.Format.wBitsPerSample == 32)
						CopyInterleaveChannel<std::int32_t>((BYTE*)bufferInfo->buffers[buffIdx], m_buffer.data(), ch, m_WaveFormat.Format.nBlockAlign, m_bufferNumFrames);
				}
			}
		}
	}

	if (m_CaptureClient)
	{
		m_CaptureClient->NotifyNewBuffer();
	}
	if (m_RenderClient)
	{
		m_RenderClient->NotifyNewBuffer();
	}

	if (m_UsingEventHandle && m_EventHandle)
	{
		SetEvent(m_EventHandle);
	}
}

void RSAsioAudioClient::UpdateChannelMap()
{
	const WORD baseChannel = m_AsioDevice.GetBaseChannelNumber();
	const WORD maxAsioChannel = m_AsioDevice.IsOutput() ? m_AsioSharedHost.GetNumOutputChannels() : m_AsioSharedHost.GetNumInputChannels();

	const WORD numRequestedChannels = m_AsioDevice.GetNumChannels();
	const WORD numBufferChannels = m_WaveFormat.Format.nChannels;

	m_ChannelMap.resize(numBufferChannels);
	if (numRequestedChannels > 0)
	{
		for (WORD i = 0; i < numBufferChannels; ++i)
		{
			const WORD wantedAsioBuffer = baseChannel + (i % numRequestedChannels);
			if (wantedAsioBuffer < maxAsioChannel)
				m_ChannelMap[i] = wantedAsioBuffer;
			else
				m_ChannelMap[i] = -1;
		}
	}
	else
	{
		for (auto& c : m_ChannelMap)
			c = -1;
	}
}
