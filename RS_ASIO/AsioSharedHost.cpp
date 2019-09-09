#include "stdafx.h"
#include "AsioSharedHost.h"

typedef HRESULT(STDAPICALLTYPE * fnPtrDllGetClassObject)(REFCLSID rClsID, REFIID riid, void **pv);

AsioSharedHost::AsioSharedHost(const CLSID& clsid, const std::string& asioDllPath)
	: m_Trampoline_bufferSwitch(*this, &AsioSharedHost::AsioCalback_bufferSwitch)
	, m_Trampoline_sampleRateDidChange(*this, &AsioSharedHost::AsioCalback_sampleRateDidChange)
	, m_Trampoline_asioMessage(*this, &AsioSharedHost::AsioCalback_asioMessage)
	, m_Trampoline_bufferSwitchTimeInfo(*this, &AsioSharedHost::AsioCalback_bufferSwitchTimeInfo)
{
	m_AsioCallbacks.bufferSwitch = m_Trampoline_bufferSwitch.GetFuncPtr();
	m_AsioCallbacks.sampleRateDidChange = m_Trampoline_sampleRateDidChange.GetFuncPtr();
	m_AsioCallbacks.asioMessage = m_Trampoline_asioMessage.GetFuncPtr();
	m_AsioCallbacks.bufferSwitchTimeInfo = m_Trampoline_bufferSwitchTimeInfo.GetFuncPtr();

	m_Module = LoadLibraryA(asioDllPath.c_str());
	if (m_Module)
	{
		fnPtrDllGetClassObject fn = (fnPtrDllGetClassObject)GetProcAddress(m_Module, "DllGetClassObject");

		IClassFactory* pClassFactory = nullptr;

		HRESULT hr = fn(clsid, __uuidof(IClassFactory), (void**)&pClassFactory);
		if (SUCCEEDED(hr) && pClassFactory)
		{
			hr = pClassFactory->CreateInstance(nullptr, clsid, (void**)&m_Driver);
			pClassFactory->Release();

			if (SUCCEEDED(hr))
			{
				HMODULE hModule = GetModuleHandle(nullptr);
				if (m_Driver->init((void*)hModule) == ASIOFalse)
				{
					DisplayCurrentError();
					m_Driver->Release();
					m_Driver = nullptr;
				}
			}
		}
	}

	if (m_Driver)
	{
		bool err = false;

		long numInputChannels = 0;
		long numOutputChannels = 0;
		if (m_Driver->getChannels(&numInputChannels, &numOutputChannels) != ASE_OK)
		{
			err = true;
		}
		else
		{
			// get channel info
			m_AsioInChannelInfo.resize(numInputChannels);
			m_AsioOutChannelInfo.resize(numOutputChannels);

			for (size_t i = 0; i < m_AsioInChannelInfo.size() && !err; ++i)
			{
				ASIOChannelInfo& ci = m_AsioInChannelInfo[i];
				ci.channel = (long)i;
				ci.isInput = ASIOTrue;
				if (m_Driver->getChannelInfo(&ci) != ASE_OK)
					err = true;
			}
			for (size_t i = 0; i < m_AsioOutChannelInfo.size() && !err; ++i)
			{
				ASIOChannelInfo& ci = m_AsioOutChannelInfo[i];
				ci.channel = (long)i;
				ci.isInput = ASIOFalse;
				if (m_Driver->getChannelInfo(&ci) != ASE_OK)
					err = true;
			}

			
		}

		if (err)
		{
			m_AsioInChannelInfo.clear();
			m_AsioOutChannelInfo.clear();

			DisplayCurrentError();
			m_Driver->Release();
			m_Driver = nullptr;
		}
	}
}

AsioSharedHost::~AsioSharedHost()
{
	if (m_Driver)
	{
		m_Driver->stop();
		m_Driver->disposeBuffers();
		m_Driver->Release();
		m_Driver = nullptr;
	}
	if (m_Module)
	{
		FreeLibrary(m_Module);
		m_Module = nullptr;
	}
}

bool AsioSharedHost::IsValid() const
{
	return m_Driver != nullptr;
}

ASIOError AsioSharedHost::Start(const WAVEFORMATEX& format, const REFERENCE_TIME& suggestedBufferDuration)
{
	std::cout << __FUNCTION__ " - startCount: " << m_StartCount << std::endl;
	if (!IsValid())
		return ASE_NotPresent;

	if (m_StartCount == 0)
	{
		// Switch ASIO sample rate if needed
		ASIOSampleRate asioSampleRate;
		if (m_Driver->getSampleRate(&asioSampleRate) != ASE_OK)
		{
			DisplayCurrentError();
			return ASE_HWMalfunction;
		}
		if (std::lround(asioSampleRate) != format.nSamplesPerSec)
		{
			std::cout << std::dec << "  Switching sample rate from " << std::lround(asioSampleRate) << " to " << format.nSamplesPerSec << "..." << std::endl;
			if (m_Driver->setSampleRate((ASIOSampleRate)format.nSamplesPerSec) != ASE_OK)
			{
				DisplayCurrentError();
				return ASE_HWMalfunction;
			}
		}

		// get buffer info
		long minBufferFrames = 0;
		long maxBufferFrames = 0;
		long preferredBufferFrames = 0;
		long bufferGranularity = 0;
		if (m_Driver->getBufferSize(&minBufferFrames, &maxBufferFrames, &preferredBufferFrames, &bufferGranularity) != ASE_OK)
		{
			DisplayCurrentError();
			return ASE_HWMalfunction;
		}

		std::cout << std::dec << "  ASIOBufferSize - min: " << minBufferFrames << " max: " << maxBufferFrames << " preferred: " << preferredBufferFrames << " granularity: " << bufferGranularity << std::endl;

		const REFERENCE_TIME minDuration = 1 * 10000;
		const REFERENCE_TIME maxDuration = 100 * 10000;
		const LONGLONG suggestedBufferDurationFrames = DurationToAudioFrames(suggestedBufferDuration, format.nSamplesPerSec);

		// clamp buffer duration
		REFERENCE_TIME bufferDuration = std::max<REFERENCE_TIME>(std::min<REFERENCE_TIME>(suggestedBufferDuration, maxDuration), minDuration);
		LONGLONG bufferDurationFrames = DurationToAudioFrames(bufferDuration, format.nSamplesPerSec);

		// make sure buffer is within parameters from ASIO info
		if (bufferDurationFrames < minBufferFrames)
			bufferDurationFrames = minBufferFrames;
		else if (bufferDurationFrames > maxBufferFrames)
			bufferDurationFrames = maxBufferFrames;
		else
		{
			// TODO: maybe handle this better in the future try trying to approximate the buffer size...
			bufferDurationFrames = preferredBufferFrames;
		}
		bufferDuration = AudioFramesToDuration(bufferDurationFrames, format.nSamplesPerSec);

		std::cout << "  suggested buffer duration: " << RefTimeToMilisecs(suggestedBufferDuration) << "ms (" << std::dec << suggestedBufferDurationFrames << " frames)" << std::endl;
		std::cout << "  actual buffer duration: " << RefTimeToMilisecs(bufferDuration) << "ms (" << std::dec << bufferDurationFrames << " frames)" << std::endl;

		// create the buffers
		m_AsioBuffers.resize(m_AsioOutChannelInfo.size() + m_AsioInChannelInfo.size());
		{
			size_t i = 0;
			for (unsigned outC = 0; outC < m_AsioOutChannelInfo.size(); ++outC, ++i)
			{
				ASIOBufferInfo& asioBuffer = m_AsioBuffers[i];
				asioBuffer.isInput = ASIOFalse;
				asioBuffer.channelNum = outC;
				asioBuffer.buffers[0] = asioBuffer.buffers[1] = nullptr;
			}
			for (unsigned inC = 0; inC < m_AsioInChannelInfo.size(); ++inC, ++i)
			{
				ASIOBufferInfo& asioBuffer = m_AsioBuffers[i];
				asioBuffer.isInput = ASIOTrue;
				asioBuffer.channelNum = inC;
				asioBuffer.buffers[0] = asioBuffer.buffers[1] = nullptr;
			}
		}

		if (m_Driver->createBuffers(m_AsioBuffers.data(), m_AsioBuffers.size(), (LONG)bufferDurationFrames, &m_AsioCallbacks) != ASE_OK)
		{
			DisplayCurrentError();

			m_AsioBuffers.clear();
			return ASE_HWMalfunction;
		}

		m_NumBufferFrames = (UINT32)bufferDurationFrames;
		m_CurrentWaveFormat = format;

		if (m_Driver->start() != ASE_OK)
		{
			DisplayCurrentError();

			if (m_Driver->disposeBuffers() != ASE_OK)
			{
				DisplayCurrentError();
			}
			m_AsioBuffers.clear();

			return ASE_HWMalfunction;
		}
	}

	++m_StartCount;

	return ASE_OK;
}

void AsioSharedHost::Stop()
{
	if (m_StartCount == 0)
	{
		std::cerr << __FUNCTION__ " - too many stop calls!" << std::endl;
		return;
	}

	--m_StartCount;

	if (m_StartCount == 0 && m_Driver)
	{
		if (m_Driver->stop() != ASE_OK)
		{
			DisplayCurrentError();
		}
		if (m_Driver->disposeBuffers() != ASE_OK)
		{
			DisplayCurrentError();
		}
		m_NumBufferFrames = 0;
		m_AsioBuffers.clear();
		m_AsioInChannelInfo.clear();
		m_AsioOutChannelInfo.clear();
	}
}

void AsioSharedHost::AddBufferSwitchListener(IAsioBufferSwitchListener* listener)
{
	std::lock_guard<std::mutex> guard(m_AsioMutex);
	m_AsioBufferListeners.emplace(listener);
}

void AsioSharedHost::RemoveBufferSwitchListener(IAsioBufferSwitchListener* listener)
{
	std::lock_guard<std::mutex> guard(m_AsioMutex);
	auto it = m_AsioBufferListeners.find(listener);
	if (it != m_AsioBufferListeners.end())
		m_AsioBufferListeners.erase(it);
}

bool AsioSharedHost::IsWaveFormatSupported(const WAVEFORMATEX& format, bool output, unsigned firstAsioChannel, unsigned numAsioChannels) const
{
	if (!IsValid())
		return false;

	long numInputChannels = 0;
	long numOutputChannels = 0;
	if (m_Driver->getChannels(&numInputChannels, &numOutputChannels) != ASE_OK)
	{
		return false;
	}

	// basic format checks
	if (format.wFormatTag != WAVE_FORMAT_PCM && format.wFormatTag != WAVE_FORMAT_EXTENSIBLE)
	{
		std::cerr << "  unknown wFormatTag: " << format.wFormatTag << std::endl;
		return false;
	}

	// check channels
	const long maxChannels = output ? numOutputChannels : numInputChannels;
	if (format.nChannels > maxChannels)
	{
		std::cerr << "  unsupported number of channels: " << format.nChannels << std::endl;
		return false;
	}

	// check sample rate
	if (m_Driver->canSampleRate((ASIOSampleRate)format.nSamplesPerSec) != ASE_OK)
	{
		std::cerr << "  unsupported sample rate: " << format.nSamplesPerSec << std::endl;
		return false;
	}

	// check bit depth
	if ((format.wBitsPerSample % 8) != 0)
	{
		std::cerr << "  bad wBitsPerSample: " << format.wBitsPerSample << std::endl;
		return false;
	}
	const std::vector<ASIOChannelInfo>& channelInfo = output ? m_AsioOutChannelInfo : m_AsioInChannelInfo;

	// make sure all channels use the same format
	ASIOSampleType sampleType;
	if (!CheckSampleTypeAcrossChannels(sampleType, output, firstAsioChannel, numAsioChannels))
	{
		std::cerr << "  requested channels use multiple different sample types" << std::endl;
		return false;
	}

	// check if format matches with bits per sample
	if (format.wBitsPerSample == 32)
	{
		switch (sampleType)
		{
			case ASIOSTInt32MSB:
			case ASIOSTFloat32MSB:
			case ASIOSTInt32MSB16:
			case ASIOSTInt32MSB18:
			case ASIOSTInt32MSB20:
			case ASIOSTInt32MSB24:
			case ASIOSTInt32LSB:
			case ASIOSTFloat32LSB:
			case ASIOSTInt32LSB16:
			case ASIOSTInt32LSB18:
			case ASIOSTInt32LSB20:
			case ASIOSTInt32LSB24:
				break;
			default:
				std::cerr << "  requested wBitsPerSample is " << format.wBitsPerSample << " but ASIO channels format is " << sampleType << std::endl;
				return false;
		}
	}
	else if (format.wBitsPerSample == 24)
	{
		switch (sampleType)
		{
			case ASIOSTInt24MSB:
			case ASIOSTInt24LSB:
				break;
			default:
				std::cerr << "  requested wBitsPerSample is " << format.wBitsPerSample << " but ASIO channels format is " << sampleType << std::endl;
				return false;
		}
	}
	else if (format.wBitsPerSample == 16)
	{
		switch (sampleType)
		{
		case ASIOSTInt16MSB:
		case ASIOSTInt16LSB:
			break;
		default:
			std::cerr << "  requested wBitsPerSample is " << format.wBitsPerSample << " but ASIO channels format is " << sampleType << std::endl;
			return false;
		}
	}
	else
	{
		std::cerr << "  requested wBitsPerSample is not supported" << std::endl;
		return false;
	}

	// block align sanity check
	const WORD expectedBlockAlign = (format.wBitsPerSample / 8) * format.nChannels;
	if (format.nBlockAlign != expectedBlockAlign)
	{
		std::cerr << "  unexpected nBlockAlign: " << format.nBlockAlign << " | expected: " << expectedBlockAlign << std::endl;
		return false;
	}

	// nAvgBytesPerSec sanity check
	const DWORD expectedBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
	if (format.nAvgBytesPerSec != expectedBytesPerSec)
	{
		std::cerr << "  unexpected nAvgBytesPerSec: " << format.nAvgBytesPerSec << " | expected: " << expectedBytesPerSec << std::endl;
		return false;
	}

	WORD bitsPerSample = format.wBitsPerSample;
	if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && format.cbSize >= 22)
	{
		const WAVEFORMATEXTENSIBLE& wfe = (const WAVEFORMATEXTENSIBLE&)format;

		// format check
		if (wfe.SubFormat != KSDATAFORMAT_SUBTYPE_PCM && wfe.SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			std::cerr << "  compressed formats are not supported" << std::endl;
			return false;
		}

		// we currently don't support IEEE float...
		if (wfe.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			std::cerr << "  IEEE Float format is not supported" << std::endl;
			return false;
		}

		// check bit depth
		if (wfe.Format.wBitsPerSample < wfe.Samples.wValidBitsPerSample)
		{
			std::cerr << "  wBitsPerSample: " << wfe.Format.wBitsPerSample << " is smaller than wValidBitsPerSample: " << wfe.Samples.wValidBitsPerSample << std::endl;
			return false;
		}
		bitsPerSample = wfe.Samples.wValidBitsPerSample;
	}

	// check bit depth
	if (bitsPerSample != 24)
	{
		std::cerr << "  bitsPerSample unsupported: " << bitsPerSample << std::endl;
		return false;
	}
	if (sampleType != ASIOSTInt32LSB)
	{
		std::cerr << "  ASIO sample type " << sampleType << " is not currently supported" << std::endl;
		return false;
	}

	return true;
}

bool AsioSharedHost::CheckSampleTypeAcrossChannels(ASIOSampleType& outType, bool output, unsigned firstAsioChannel, unsigned numAsioChannels) const
{
	const std::vector<ASIOChannelInfo>& channelInfo = output ? m_AsioOutChannelInfo : m_AsioInChannelInfo;
	const size_t n = channelInfo.size();

	if ((firstAsioChannel + numAsioChannels) > n || numAsioChannels == 0)
		return false;

	ASIOSampleType type;
	for (size_t i = 0; i < numAsioChannels; ++i)
	{
		const ASIOChannelInfo& info = channelInfo[i + firstAsioChannel];

		if (i == 0)
		{
			type = info.type;
		}
		else if (info.type != type)
		{
			return false;
		}
	}

	outType = type;
	return true;
}

UINT32 AsioSharedHost::GetBufferNumFrames() const
{
	return m_NumBufferFrames;
}

bool AsioSharedHost::GetLatencyTime(REFERENCE_TIME& in, REFERENCE_TIME& out)
{
	if (!IsValid() || m_StartCount==0)
		return false;

	long inputLatency = 0;
	long outputLatency = 0;
	if (m_Driver->getLatencies(&inputLatency, &outputLatency) != ASE_OK)
		return false;

	in = AudioFramesToDuration(inputLatency, m_CurrentWaveFormat.nSamplesPerSec);
	out = AudioFramesToDuration(outputLatency, m_CurrentWaveFormat.nSamplesPerSec);

	return true;
}

ASIOBufferInfo* AsioSharedHost::GetOutputBuffer(unsigned channel)
{
	if (m_StartCount == 0)
		return nullptr;

	if (channel >= m_AsioOutChannelInfo.size())
		return nullptr;

	return &m_AsioBuffers[channel];
}

ASIOBufferInfo* AsioSharedHost::GetInputBuffer(unsigned channel)
{
	if (m_StartCount == 0)
		return nullptr;

	if (channel >= m_AsioInChannelInfo.size())
		return nullptr;

	return &m_AsioBuffers[m_AsioOutChannelInfo.size() + channel];
}

void AsioSharedHost::DisplayCurrentError()
{
	if (!m_Driver)
		return;

	char err[128];
	m_Driver->getErrorMessage(err);

	std::cerr << "ASIO Error: " << err << std::endl;
}

void __cdecl AsioSharedHost::AsioCalback_bufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
{
	std::lock_guard<std::mutex> guard(m_AsioMutex);

	// zero output
	const unsigned numBufferBytes = m_NumBufferFrames * m_CurrentWaveFormat.nBlockAlign;
	const size_t numOuts = m_AsioOutChannelInfo.size();
	for (size_t i = 0; i < numOuts; ++i)
	{
		ASIOBufferInfo* asioBuffer = GetOutputBuffer(i);
		if (asioBuffer)
		{
			memset(asioBuffer->buffers[doubleBufferIndex], 0, numBufferBytes);
		}
	}


	for (IAsioBufferSwitchListener* listener : m_AsioBufferListeners)
	{
		listener->OnAsioBufferSwitch((unsigned)doubleBufferIndex);
	}
}

void __cdecl AsioSharedHost::AsioCalback_sampleRateDidChange(ASIOSampleRate sRate)
{
	std::cout << __FUNCTION__ << std::endl;
}

long __cdecl AsioSharedHost::AsioCalback_asioMessage(long selector, long value, void* message, double* opt)
{
	std::cout << __FUNCTION__ << std::endl;

	return 0;
}

ASIOTime* __cdecl AsioSharedHost::AsioCalback_bufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
{
	std::cout << __FUNCTION__ << std::endl;

	return nullptr;
}
