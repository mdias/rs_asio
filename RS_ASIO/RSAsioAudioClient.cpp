#include "stdafx.h"
#include "AsioSharedHost.h"
#include "RSAsioDevice.h"
#include "RSAsioAudioClient.h"
#include "RSAsioAudioRenderClient.h"
#include "RSAsioAudioCaptureClient.h"
#include "AudioProcessing.h"

RSAsioAudioClient::RSAsioAudioClient(RSAsioDevice& asioDevice)
	: m_AsioDevice(asioDevice)
	, m_AsioSharedHost(m_AsioDevice.GetAsioHost())
{
	m_AsioDevice.AddRef();
	m_AsioSharedHost.AddRef();

	m_MyUnknown = new MyUnknown();
	m_MyUnknown->RefCountHackEnabled = asioDevice.GetConfig().enableRefCountHack.value_or(m_AsioSharedHost.GetIsAsio4All());

	memset(&m_WaveFormat, 0, sizeof(m_WaveFormat));
}

RSAsioAudioClient::~RSAsioAudioClient()
{
	rslog::info_ts() << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << std::endl;

	m_MyUnknown->Release();

	Stop();
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
	else if (riid == __uuidof(MyUnknown))
	{
		*ppvObject = m_MyUnknown;
		m_MyUnknown->AddRef();
		return S_OK;
	}

	return ComBaseUnknown<IAudioClient3>::QueryInterface(riid, ppvObject);
}

HRESULT RSAsioAudioClient::Initialize(AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid)
{
	std::lock_guard<std::mutex> g(m_controlMutex);

	const bool useEventCallback = StreamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

	static bool isFirstTimeCalled = true;
	if (isFirstTimeCalled)
	{
		if (!useEventCallback)
		{
			MessageBox(GetGameWindow(), TEXT("Tried to initialize audio without using an event callback.\nDid you set Win32UltraLowLatencyMode=1 in Rocksmith.ini?"), TEXT("RS-ASIO Error"), MB_OK | MB_ICONERROR);
		}
		isFirstTimeCalled = false;
	}

	if (!pFormat)
		return E_POINTER;

	if (m_IsInitialized)
		return AUDCLNT_E_ALREADY_INITIALIZED;

	if (useEventCallback && hnsBufferDuration != hnsPeriodicity)
		return AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (FAILED(IsFormatSupported(ShareMode, pFormat, nullptr)))
		return AUDCLNT_E_UNSUPPORTED_FORMAT;

	if (m_AsioSharedHost.SetSamplerate(pFormat->nSamplesPerSec) != ASE_OK)
		return E_FAIL;

	rslog::info_ts() << std::dec << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << " - host requested buffer duration: " << RefTimeToMilisecs(hnsBufferDuration) << "ms (" << std::dec << DurationToAudioFrames(hnsBufferDuration, pFormat->nSamplesPerSec) << " frames)" << std::endl;
	rslog::info_ts() << m_AsioDevice.GetIdRef() << " " << (*pFormat);

	// calculate buffer duration
	DWORD bufferDurationFrames = (DWORD)DurationToAudioFrames(hnsBufferDuration, pFormat->nSamplesPerSec);
	switch (m_AsioDevice.GetConfig().bufferSizeMode)
	{
		case RSAsioDevice::BufferSizeMode_Host:
			break;
		case RSAsioDevice::BufferSizeMode_Driver:
			if (!m_AsioSharedHost.GetPreferredBufferSize(bufferDurationFrames))
			{
				rslog::error_ts() << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << " - Couldn't get driver preferred buffer size" << std::endl;
				return E_FAIL;
			}
			break;
		case RSAsioDevice::BufferSizeMode_Custom:
			bufferDurationFrames = m_AsioDevice.GetConfig().customBufferSize;
			break;
		default:
			rslog::error_ts() << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << " - unhandled buffer size mode" << std::endl;
			return E_INVALIDARG;
	}

	// clamp the buffer size to device limits if we're not using the preferred size from the driver
	if (m_AsioDevice.GetConfig().bufferSizeMode != RSAsioDevice::BufferSizeMode_Driver)
	{
		if (!m_AsioSharedHost.ClampBufferSizeToLimits(bufferDurationFrames))
		{
			rslog::error_ts() << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << " - Couldn't clamp buffer size to limits" << std::endl;
			return E_FAIL;
		}
	}

	rslog::info_ts() << std::dec << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << " - actual buffer duration: " << RefTimeToMilisecs(AudioFramesToDuration(bufferDurationFrames, pFormat->nSamplesPerSec)) << "ms (" << std::dec << bufferDurationFrames << " frames)" << std::endl;

	// setup ASIO streaming
	if (m_AsioSharedHost.Setup(*pFormat, bufferDurationFrames) != ASE_OK)
		return E_FAIL;

	memset(&m_WaveFormat, 0, sizeof(m_WaveFormat));
	if (pFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		m_WaveFormat = *(WAVEFORMATEXTENSIBLE*)pFormat;
		m_WaveFormatIsFloat = (m_WaveFormat.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
	}
	else
	{
		m_WaveFormat.Format = *pFormat;
	}

	UpdateChannelMap();

	if (m_AsioDevice.GetConfig().isOutput)
	{
		m_RenderClient = new RSAsioAudioRenderClient(*this);
	}
	else
	{
		m_CaptureClient = new RSAsioAudioCaptureClient(*this);
	}

	UINT32 numFrames = m_AsioSharedHost.GetBufferNumFrames();
	m_bufferNumFrames = numFrames;
	m_frontBuffer.resize(pFormat->nBlockAlign * numFrames);
	m_backBuffer.resize(pFormat->nBlockAlign * numFrames);
	memset(m_frontBuffer.data(), 0, m_frontBuffer.size());
	memset(m_backBuffer.data(), 0, m_backBuffer.size());
	
	m_UsingEventHandle = useEventCallback;
	m_IsInitialized = true;
	m_BuffersWereSwapped = false;

	m_AsioSharedHost.AddBufferSwitchListener(this);

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

	*phnsLatency = m_AsioDevice.GetConfig().isOutput ? out : in;

	return S_OK;
}

HRESULT RSAsioAudioClient::GetCurrentPadding(UINT32 *pNumPaddingFrames)
{
	if (!pNumPaddingFrames)
		return E_POINTER;

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	bool isBufferWaiting = false;

	if (m_AsioDevice.GetConfig().isOutput)
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

	static bool isFirstTimeCalled = true;
	if (isFirstTimeCalled)
	{
		if (ShareMode != AUDCLNT_SHAREMODE_EXCLUSIVE)
		{
			MessageBox(GetGameWindow(), TEXT("Tried to initialize audio without exclusivity.\nDid you set ExclusiveMode=1 in Rocksmith.ini?"), TEXT("RS-ASIO Error"), MB_OK | MB_ICONERROR);
		}
		isFirstTimeCalled = false;
	}

	// we only support exclusive mode
	if (ShareMode != AUDCLNT_SHAREMODE_EXCLUSIVE)
	{
		rslog::error_ts() << "  shared mode is not supported" << std::endl;
		return AUDCLNT_E_UNSUPPORTED_FORMAT;
	}

	// check channels
	if (pFormat->nChannels > m_AsioDevice.GetNumWasapiChannels())
	{
		rslog::error_ts() << "  unsupported number of channels: " << pFormat->nChannels << std::endl;
		return AUDCLNT_E_UNSUPPORTED_FORMAT;
	}
	if (m_AsioDevice.GetConfig().numAsioChannels == 0)
	{
		rslog::error_ts() << "  endpoint has no configured channels" << std::endl;
		return AUDCLNT_E_UNSUPPORTED_FORMAT;
	}

	// check compatibility with asio host
	if (!m_AsioSharedHost.IsWaveFormatSupported(*pFormat, m_AsioDevice.GetConfig().isOutput, m_AsioDevice.GetConfig().baseAsioChannelNumber, m_AsioDevice.GetConfig().numAsioChannels))
	{
		rslog::info_ts() << "  requested format is not supported" << std::endl;
		return AUDCLNT_E_UNSUPPORTED_FORMAT;
	}

	return S_OK;
}

HRESULT RSAsioAudioClient::GetMixFormat(WAVEFORMATEX **ppDeviceFormat)
{
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
	std::lock_guard<std::mutex> g(m_controlMutex);

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (m_UsingEventHandle && !m_EventHandle)
		return AUDCLNT_E_EVENTHANDLE_NOT_SET;

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (m_IsStarted)
		return AUDCLNT_E_NOT_STOPPED;

	m_IsStarted = true;
	m_IsFirstBuffer = true;
	m_dbgNumBufferSwitches = 0;

	// start ASIO streaming
	if (m_AsioSharedHost.Start() != ASE_OK)
	{
		m_IsStarted = false;
		return E_FAIL;
	}

	return S_OK;
}

HRESULT RSAsioAudioClient::Stop()
{
	std::lock_guard<std::mutex> g(m_controlMutex);

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (m_IsStarted)
	{
		m_AsioSharedHost.Stop();
	}

	// just in case calls to SwapBufers are still pending etc...
	std::lock_guard<std::mutex> g2(m_bufferMutex);

	m_IsStarted = false;

	return S_OK;
}

HRESULT RSAsioAudioClient::Reset()
{
	std::lock_guard<std::mutex> g(m_controlMutex);

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (m_IsStarted)
		return AUDCLNT_E_NOT_STOPPED;

	return S_OK;
}

HRESULT RSAsioAudioClient::SetEventHandle(HANDLE eventHandle)
{
	std::lock_guard<std::mutex> g(m_controlMutex);

	if (!m_AsioSharedHost.IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (!m_IsInitialized)
		return AUDCLNT_E_NOT_INITIALIZED;

	if (!m_UsingEventHandle)
		return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;

	// the docs don't mention which error to return - if any - when we attempt to set the event handle
	// after calling Start()
	if (m_IsStarted)
		return E_FAIL;

	m_EventHandle = eventHandle;

	return S_OK;
}

HRESULT RSAsioAudioClient::GetService(REFIID riid, void **ppv)
{
	if (riid == __uuidof(IAudioRenderClient))
	{
		if (m_RenderClient)
		{
			rslog::info_ts() << "  returning render client" << std::endl;
			m_RenderClient->AddRef();
			*ppv = m_RenderClient;
			return S_OK;
		}
	}
	else if (riid == __uuidof(IAudioCaptureClient))
	{
		if (m_CaptureClient)
		{
			rslog::info_ts() << "  returning capture client" << std::endl;
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

void RSAsioAudioClient::SwapBuffers()
{
	std::lock_guard<std::mutex> g(m_bufferMutex);
	std::swap(m_frontBuffer, m_backBuffer);
	m_BuffersWereSwapped = true;

	if (m_dbgNumBufferSwitches < 3)
		rslog::info_ts() << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << std::endl;
}

void RSAsioAudioClient::OnAsioBufferSwitch(unsigned buffIdx)
{
	// NOTE: we shouldn't need to lock the m_controlMutex to read stuff like m_IsStarted
	// because calls to Start() should have these members set already, and calls to Stop()
	// should have the driver wait for any pending call to OnAsioBufferSwitch anyway.

	if (!m_bufferMutex.try_lock())
	{
		rslog::info_ts() << m_AsioDevice.GetIdRef() << " " << __FUNCTION__ << " - failed to get lock first time. This might be harmless, but if we freeze here this is likely related" << std::endl;
		m_bufferMutex.lock();
	}

	std::lock_guard<std::mutex> g(m_bufferMutex, std::adopt_lock);

	const bool isOutput = m_AsioDevice.GetConfig().isOutput;
	const bool isInput = !isOutput;

	if (!m_IsStarted)
	{
		memset(m_frontBuffer.data(), 0, m_frontBuffer.size());
		if (isInput)
		{
			return;
		}
	}

	// sanity check
	if (isOutput)
	{
		if (m_ChannelMap.size() != m_AsioSharedHost.GetNumOutputChannels())
			return;
	}
	else
	{
		if (m_ChannelMap.size() < m_WaveFormat.Format.nChannels)
			return;
	}

	// get game sample type
	ASIOSampleType gameSampleType = ASIOSTFloat32LSB;
	if (!AsioSampleTypeFromFormat(&gameSampleType, m_WaveFormat.Format.wBitsPerSample, m_WaveFormatIsFloat))
		return;
	const WORD gameSampleTypeSize = GetAsioSampleTypeNumBytes(gameSampleType);
	if (!gameSampleTypeSize)
		return;

	// check if and how we want to do software volume processing
	float fSoftwareVolumeScalar = 1.0f;
	bool doSoftwareVolume = m_AsioDevice.GetSoftwareVolumeScalar(&fSoftwareVolumeScalar);
	
	if (isOutput)
	{
		// no need to do software volume if we're streaming silence
		if (m_BuffersWereSwapped)
		{
			if (doSoftwareVolume)
			{
				const DWORD totalSamples = m_bufferNumFrames * m_WaveFormat.Format.nChannels;
				AudioProcessing::DoSoftwareVolumeDsp(m_frontBuffer.data(), gameSampleType, totalSamples, fSoftwareVolumeScalar);
			}
		}

		for (DWORD asioCh = 0; asioCh < m_ChannelMap.size(); ++asioCh)
		{
			const ASIOChannelInfo* asioChannelInfo = m_AsioSharedHost.GetOutputChannelInfo(asioCh);
			ASIOBufferInfo* bufferInfo = m_AsioSharedHost.GetOutputBuffer(asioCh);
			if (bufferInfo && asioChannelInfo)
			{
				const WORD asioSampleTypeSize = GetAsioSampleTypeNumBytes(asioChannelInfo->type);
				const int srcCh = m_ChannelMap[asioCh];
				if (asioSampleTypeSize)
				{
					if (srcCh >= 0)
					{
						AudioProcessing::CopyConvertFormat(
							m_frontBuffer.data() + srcCh * gameSampleTypeSize, gameSampleType, m_WaveFormat.Format.nBlockAlign,
							m_bufferNumFrames,
							(BYTE*)bufferInfo->buffers[buffIdx], asioChannelInfo->type, asioSampleTypeSize
						);
					}
					else
					{
						// output silence to unassigned channels
						memset(bufferInfo->buffers[buffIdx], 0, asioSampleTypeSize * m_bufferNumFrames);
					}
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
				const ASIOChannelInfo* asioChannelInfo = m_AsioSharedHost.GetInputChannelInfo(asioCh);
				ASIOBufferInfo* bufferInfo = m_AsioSharedHost.GetInputBuffer(asioCh);
				if (bufferInfo && asioChannelInfo)
				{
					const WORD asioSampleTypeSize = GetAsioSampleTypeNumBytes(asioChannelInfo->type);

					if (asioSampleTypeSize)
					{
						AudioProcessing::CopyConvertFormat(
							(BYTE*)bufferInfo->buffers[buffIdx], asioChannelInfo->type, asioSampleTypeSize,
							m_bufferNumFrames,
							m_frontBuffer.data() + ch * gameSampleTypeSize, gameSampleType, m_WaveFormat.Format.nBlockAlign
						);
					}
				}
			}
		}

		if (doSoftwareVolume)
		{
			const DWORD totalSamples = m_bufferNumFrames * m_WaveFormat.Format.nChannels;
			AudioProcessing::DoSoftwareVolumeDsp(m_frontBuffer.data(), gameSampleType, totalSamples, fSoftwareVolumeScalar);
		}
	}

	if (m_IsStarted)
	{
		if (m_CaptureClient)
		{
			m_CaptureClient->NotifyNewBuffer();
		}
		if (m_RenderClient)
		{
			m_RenderClient->NotifyNewBuffer();
		}

		if (m_dbgNumBufferSwitches < 3)
		{
			++m_dbgNumBufferSwitches;
		}

		// NOTE(1): we cannot notify the application multiple times otherwise it will ask for all those
		// buffers and since we don't save old buffers, it will eventually error out. Some interfaces
		// will trigger this behavior quickly.
		// NOTE(2): for input we want to force signal the app for the first incoming buffer.
		const bool signalApplicationAboutNewBuffer = m_BuffersWereSwapped || (isInput && m_IsFirstBuffer);

		m_BuffersWereSwapped = false;
		m_IsFirstBuffer = false;

		// notifying the host application should be the last thing we do
		const bool signalEvent = (m_UsingEventHandle && m_EventHandle && signalApplicationAboutNewBuffer);
		if (signalEvent)
		{
			SetEvent(m_EventHandle);
		}
	}
}

void RSAsioAudioClient::UpdateChannelMap()
{
	const bool isOutput = m_AsioDevice.GetConfig().isOutput;
	const WORD baseChannel = m_AsioDevice.GetConfig().baseAsioChannelNumber;
	const WORD altOutputBaseChannel = m_AsioDevice.GetConfig().altOutputBaseAsioChannelNumber.value_or((unsigned)baseChannel);
	const WORD maxAsioChannel = isOutput ? m_AsioSharedHost.GetNumOutputChannels() : m_AsioSharedHost.GetNumInputChannels();

	const WORD numRequestedChannels = m_AsioDevice.GetConfig().numAsioChannels;
	const WORD numBufferChannels = m_WaveFormat.Format.nChannels;

	m_ChannelMap.resize(isOutput ? maxAsioChannel : numBufferChannels);
	for (auto& c : m_ChannelMap)
		c = -1;

	if (isOutput)
	{
		WORD i = baseChannel;
		WORD srcChannel = 0;
		while (i < m_ChannelMap.size() && srcChannel < numRequestedChannels)
		{
			m_ChannelMap[i] = srcChannel;
			++srcChannel;
			++i;
		}

		if (altOutputBaseChannel != baseChannel)
		{
			i = altOutputBaseChannel;
			srcChannel = 0;
			while (i < m_ChannelMap.size() && srcChannel < numRequestedChannels)
			{
				// don't overwrite primary base channel
				if (m_ChannelMap[i] == -1)
					m_ChannelMap[i] = srcChannel;
				++srcChannel;
				++i;
			}
		}
	}
	else
	{
		for (WORD i = 0; i < numBufferChannels; ++i)
		{
			const WORD wantedAsioBuffer = baseChannel + (i % numRequestedChannels);
			if (wantedAsioBuffer < maxAsioChannel)
				m_ChannelMap[i] = wantedAsioBuffer;
		}
	}
}
