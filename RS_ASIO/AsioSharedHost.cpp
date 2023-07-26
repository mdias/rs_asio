#include "stdafx.h"
#include "AsioSharedHost.h"

#define TEST_OUTPUT_NOISE 0
#define TEST_INPUT_NOISE 0

typedef HRESULT(STDAPICALLTYPE * fnPtrDllGetClassObject)(REFCLSID rClsID, REFIID riid, void **pv);

AsioSharedHost::AsioSharedHost(const CLSID& clsid, const std::string& asioDllPath)
	: m_Trampoline_bufferSwitch(*this, &AsioSharedHost::AsioCalback_bufferSwitch)
	, m_Trampoline_sampleRateDidChange(*this, &AsioSharedHost::AsioCalback_sampleRateDidChange)
	, m_Trampoline_asioMessage(*this, &AsioSharedHost::AsioCalback_asioMessage)
	, m_Trampoline_bufferSwitchTimeInfo(*this, &AsioSharedHost::AsioCalback_bufferSwitchTimeInfo)
	, m_AsioDllPath(asioDllPath)
{
	rslog::info_ts() << "Creating AsioSharedHost - dll: " << m_AsioDllPath << std::endl;

	m_AsioCallbacks.bufferSwitch = m_Trampoline_bufferSwitch.GetFuncPtr();
	m_AsioCallbacks.sampleRateDidChange = m_Trampoline_sampleRateDidChange.GetFuncPtr();
	m_AsioCallbacks.asioMessage = m_Trampoline_asioMessage.GetFuncPtr();
	m_AsioCallbacks.bufferSwitchTimeInfo = m_Trampoline_bufferSwitchTimeInfo.GetFuncPtr();

	memset(&m_CurrentWaveFormat, 0, sizeof(m_CurrentWaveFormat));

	m_Module = LoadLibraryA(asioDllPath.c_str());
	if (m_Module)
	{
		fnPtrDllGetClassObject fn = (fnPtrDllGetClassObject)GetProcAddress(m_Module, "DllGetClassObject");

		if (!fn)
		{
			rslog::error_ts() << "Failed to GetProcAddress for DllGetClassObject() in ASIO driver "
				<< m_AsioDllPath << std::endl << "\tBad ASIO driver?" << std::endl;
				
			return;
		}

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
			else
			{
				rslog::error_ts() << "Failed to create instance of ASIO driver " << clsid << " "
					<< m_AsioDllPath << std::endl << "\tBad ASIO driver?" << std::endl;
				return;
			}
		}
		else
		{
			rslog::error_ts() << "Failed to get class factory " << clsid << " from ASIO driver "
				<< m_AsioDllPath << std::endl << "\tBad ASIO driver?" << std::endl;
			return;
		}
		
		std::string pathLowercase = asioDllPath;
		std::transform(pathLowercase.begin(), pathLowercase.end(), pathLowercase.begin(),
			[](unsigned char c) { return std::tolower(c); } 
		);

		m_AsioDllIsAsio4all = (pathLowercase.find("asio4all.dll") != std::string::npos);
		if (m_AsioDllIsAsio4all)
		{
			rslog::info_ts() << "  info: asio4all detected" << std::endl;
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
				{
					err = true;
					DisplayCurrentError();
				}
			}
			for (size_t i = 0; i < m_AsioOutChannelInfo.size() && !err; ++i)
			{
				ASIOChannelInfo& ci = m_AsioOutChannelInfo[i];
				ci.channel = (long)i;
				ci.isInput = ASIOFalse;
				if (m_Driver->getChannelInfo(&ci) != ASE_OK)
				{
					err = true;
					DisplayCurrentError();
				}
			}

			char tmpName[128];
			m_Driver->getDriverName(tmpName);

			m_DriverName = tmpName;

			// display channel information
			rslog::info_ts() << "  ASIO input channels info:" << std::endl;
			for (size_t i = 0; i < m_AsioInChannelInfo.size(); ++i)
			{
				const ASIOChannelInfo& ci = m_AsioInChannelInfo[i];
				rslog::info_ts() << "    " << i << " - active: " << ci.isActive << ", channel: " << ci.channel << ", group: " << ci.channelGroup << ", isInput: " << ci.isInput << ", type: " << ci.type << ", name: " << ci.name << std::endl;
			}
			rslog::info_ts() << "  ASIO output channels info:" << std::endl;
			for (size_t i = 0; i < m_AsioOutChannelInfo.size(); ++i)
			{
				const ASIOChannelInfo& ci = m_AsioOutChannelInfo[i];
				rslog::info_ts() << "    " << i << " - active: " << ci.isActive << ", channel: " << ci.channel << ", group: " << ci.channelGroup << ", isInput: " << ci.isInput << ", type: " << ci.type << ", name: " << ci.name << std::endl;
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
	rslog::info_ts() << "Destroying AsioSharedHost - dll: " << m_AsioDllPath << std::endl;

	if (m_Driver)
	{
		Reset();

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

ASIOError AsioSharedHost::SetSamplerate(const DWORD rate)
{
	if (!IsValid())
		return ASE_NotPresent;

	// Switch ASIO sample rate if needed
	ASIOSampleRate asioSampleRate;
	if (m_Driver->getSampleRate(&asioSampleRate) != ASE_OK)
	{
		DisplayCurrentError();
		return ASE_HWMalfunction;
	}
	if (std::lround(asioSampleRate) != rate)
	{
		if (m_IsSetup)
		{
			return ASE_HWMalfunction;
		}
		else
		{
			const ASIOSampleRate targetSampleRate = (ASIOSampleRate)rate;

			rslog::info_ts() << std::dec << "  Switching sample rate from " << std::lround(asioSampleRate) << " to " << rate << "..." << std::endl;
			if (m_Driver->setSampleRate(targetSampleRate) != ASE_OK)
			{
				DisplayCurrentError();
				return ASE_HWMalfunction;
			}
			else
			{
				// save previously selected sample rate so that we can restore it later
				m_SampleRateRestore = asioSampleRate;
			}

			rslog::info_ts() << " Waiting 5 seconds to give driver time to refresh" << std::endl;
			Sleep(5000);
		}
	}

	return ASE_OK;
}

ASIOError AsioSharedHost::Setup(const WAVEFORMATEX& format, const DWORD bufferDurationFrames)
{
	rslog::info_ts() << __FUNCTION__ " - startCount: " << m_StartCount << std::endl;

	if (!IsValid())
		return ASE_NotPresent;

	if (m_IsSetup)
	{
		if (format.nSamplesPerSec != m_CurrentWaveFormat.Format.nSamplesPerSec ||
			format.wFormatTag != m_CurrentWaveFormat.Format.wFormatTag)
		{
			rslog::error_ts() << "  wave format is different from a previous call to Setup()." << std::endl;
			return ASE_InvalidMode;
		}

		if (bufferDurationFrames != m_NumBufferFrames)
		{
			rslog::error_ts() << "  buffer duration is different from a previous call to Setup()." << std::endl;
			return ASE_InvalidMode;
		}
	}
	else
	{
		// make sure all channels are using a supported format for now
		if (m_AsioInChannelInfo.size())
		{
			if (!IsWaveFormatSupported(format, false, 0, m_AsioInChannelInfo.size()))
			{
				rslog::error_ts() << "  wave format not supported on inputs." << std::endl;
				return ASE_HWMalfunction;
			}
		}
		if (m_AsioOutChannelInfo.size())
		{
			if (!IsWaveFormatSupported(format, true, 0, m_AsioOutChannelInfo.size()))
			{
				rslog::error_ts() << "  wave format not supported on outputs." << std::endl;
				return ASE_HWMalfunction;
			}
		}

		// Ensure ASIO sample rate is correct
		ASIOSampleRate asioSampleRate;
		if (m_Driver->getSampleRate(&asioSampleRate) != ASE_OK)
		{
			DisplayCurrentError();
			return ASE_HWMalfunction;
		}
		if (std::lround(asioSampleRate) != format.nSamplesPerSec)
		{
			rslog::error_ts() << std::dec << "  Current sample rate is " << std::lround(asioSampleRate) << " which is not the expected " << format.nSamplesPerSec << std::endl;
			return ASE_InvalidMode;
		}

		// check if driver wants to be notified about output being ready
		m_PostOutputReady = (m_Driver->outputReady() == ASE_OK);
		rslog::info_ts() << "  post output ready: " << m_PostOutputReady << std::endl;

		// get buffer info
		long minAsioBufferFrames = 0;
		long maxAsioBufferFrames = 0;
		long preferredAsioBufferFrames = 0;
		long asioBufferGranularity = 0;
		if (m_Driver->getBufferSize(&minAsioBufferFrames, &maxAsioBufferFrames, &preferredAsioBufferFrames, &asioBufferGranularity) != ASE_OK)
		{
			DisplayCurrentError();
			return ASE_HWMalfunction;
		}

		rslog::info_ts() << std::dec << "  ASIOBufferSize - min: " << minAsioBufferFrames << " max: " << maxAsioBufferFrames << " preferred: " << preferredAsioBufferFrames << " granularity: " << asioBufferGranularity << std::endl;

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

		rslog::info_ts() << "  Creating ASIO buffers (" << m_AsioOutChannelInfo.size() << " out, " << m_AsioInChannelInfo.size() << " in)..." << std::endl;
		if (m_Driver->createBuffers(m_AsioBuffers.data(), m_AsioBuffers.size(), (LONG)bufferDurationFrames, &m_AsioCallbacks) != ASE_OK)
		{
			rslog::error_ts() << "  Failed to create ASIO buffers" << std::endl;
			DisplayCurrentError();

			m_AsioBuffers.clear();
			return ASE_HWMalfunction;
		}

		// store config
		m_NumBufferFrames = (UINT32)bufferDurationFrames;
		if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
			m_CurrentWaveFormat = (WAVEFORMATEXTENSIBLE&)format;
		else
			m_CurrentWaveFormat.Format = format;

		m_IsSetup = true;
	}

	return ASE_OK;
}

void AsioSharedHost::Reset()
{
	if (!m_IsSetup)
		return;

	if (m_Driver->stop() != ASE_OK)
	{
		DisplayCurrentError();
	}
	std::lock_guard<std::mutex> guard(m_AsioMutex);
	if (m_Driver->disposeBuffers() != ASE_OK)
	{
		DisplayCurrentError();
	}
	m_AsioBuffers.clear();
	memset(&m_CurrentWaveFormat, 0, sizeof(m_CurrentWaveFormat));

	// restore sample rate
	if (m_SampleRateRestore.has_value())
	{
		rslog::info_ts() << "Restoring previously set sample rate" << std::endl;
		if (m_Driver->setSampleRate(m_SampleRateRestore.value()) != ASE_OK)
		{
			DisplayCurrentError();
		}
		m_SampleRateRestore.reset();
	}

	m_IsSetup = false;
}

ASIOError AsioSharedHost::Start()
{
	rslog::info_ts() << __FUNCTION__ " - enter startCount: " << m_StartCount << std::endl;

	if (!IsValid() || !m_IsSetup)
		return ASE_NotPresent;

	if (m_StartCount == 0)
	{
		m_dbgNumBufferSwitches = 0;
		
		rslog::info_ts() << "  Starting ASIO stream..." << std::endl;
		if (m_Driver->start() != ASE_OK)
		{
			rslog::error_ts() << "  Failed to start ASIO stream" << std::endl;
			DisplayCurrentError();

			return ASE_HWMalfunction;
		}
	}

	++m_StartCount;

	rslog::info_ts() << __FUNCTION__ " - leave startCount: " << m_StartCount << std::endl;

	return ASE_OK;
}

void AsioSharedHost::Stop()
{
	if (!m_AsioMutex.try_lock())
	{
		rslog::info_ts() << m_DriverName << " " __FUNCTION__ " - failed to get lock first time. This might be harmless, but if we freeze here this is likely related" << std::endl;
		m_AsioMutex.lock();
	}

	std::lock_guard<std::mutex> guard(m_AsioMutex, std::adopt_lock);

	rslog::info_ts() << __FUNCTION__ " - enter startCount: " << m_StartCount << std::endl;

	if (m_StartCount == 0)
	{
		rslog::error_ts() << __FUNCTION__ " - too many stop calls!" << std::endl;
		return;
	}
	else if (m_StartCount == 1)
	{
		rslog::info_ts() << __FUNCTION__ " - stopping ASIO stream" << std::endl;
		if (m_Driver->stop() != ASE_OK)
		{
			rslog::error_ts() << "  Failed to stop ASIO stream" << std::endl;
			DisplayCurrentError();
		}
	}

	--m_StartCount;

	rslog::info_ts() << __FUNCTION__ " - leave startCount: " << m_StartCount << std::endl;
}

bool AsioSharedHost::GetPreferredBufferSize(DWORD& outBufferSizeFrames) const
{
	if (!IsValid())
		return false;

	if (m_IsSetup)
	{
		outBufferSizeFrames = m_NumBufferFrames;
		return true;
	}

	// get buffer info
	long minAsioBufferFrames = 0;
	long maxAsioBufferFrames = 0;
	long preferredAsioBufferFrames = 0;
	long asioBufferGranularity = 0;
	if (m_Driver->getBufferSize(&minAsioBufferFrames, &maxAsioBufferFrames, &preferredAsioBufferFrames, &asioBufferGranularity) != ASE_OK)
	{
		DisplayCurrentError();
		return false;
	}

	outBufferSizeFrames = (DWORD)preferredAsioBufferFrames;

	return true;
}

bool AsioSharedHost::ClampBufferSizeToLimits(DWORD& inOutBufferSizeFrames) const
{
	if (!IsValid())
		return false;

	// get buffer info
	long minAsioBufferFrames = 0;
	long maxAsioBufferFrames = 0;
	long preferredAsioBufferFrames = 0;
	long asioBufferGranularity = 0;
	if (m_Driver->getBufferSize(&minAsioBufferFrames, &maxAsioBufferFrames, &preferredAsioBufferFrames, &asioBufferGranularity) != ASE_OK)
	{
		DisplayCurrentError();
		return false;
	}

	if (minAsioBufferFrames < 0 || maxAsioBufferFrames < 0)
	{
		return false;
	}

	if (inOutBufferSizeFrames < (DWORD)minAsioBufferFrames)
		inOutBufferSizeFrames = (DWORD)minAsioBufferFrames;
	else if (inOutBufferSizeFrames > (DWORD)maxAsioBufferFrames)
		inOutBufferSizeFrames = (DWORD)minAsioBufferFrames;

	if (asioBufferGranularity > 1)
	{
		const DWORD mod = inOutBufferSizeFrames % asioBufferGranularity;
		if (mod != 0)
		{
			inOutBufferSizeFrames += asioBufferGranularity - mod;
		}
	}
	else if (asioBufferGranularity == -1)
	{
		DWORD pow2size = 1;
		while (pow2size < inOutBufferSizeFrames && pow2size < (DWORD)maxAsioBufferFrames)
		{
			pow2size *= 2;
		}

		inOutBufferSizeFrames = pow2size;

		if (inOutBufferSizeFrames > (DWORD)maxAsioBufferFrames)
			inOutBufferSizeFrames = maxAsioBufferFrames;
	}

	return true;
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
		rslog::error_ts() << "  ASIO: failed to obtain number of channels" << std::endl;
		return false;
	}

	// basic format checks
	if (format.wFormatTag != WAVE_FORMAT_PCM && format.wFormatTag != WAVE_FORMAT_EXTENSIBLE)
	{
		rslog::error_ts() << "  unknown wFormatTag: " << format.wFormatTag << std::endl;
		return false;
	}

	// check channels
	const long maxChannels = output ? numOutputChannels : numInputChannels;
	const long maxUsedChannel = firstAsioChannel + numAsioChannels;
	const long actualMaxUsedChannel = std::min<long>(maxChannels, maxUsedChannel);
	const long actualNumAsioChannels = actualMaxUsedChannel - firstAsioChannel;

	if (maxUsedChannel > maxChannels)
	{
		rslog::info_ts() << "  WARNING: max requested channel " << maxUsedChannel << " is beyond the max asio channels available " << maxChannels << std::endl;
	}

	if (actualNumAsioChannels < 0)
	{
		rslog::error_ts() << "  no valid channels" << std::endl;
		return false;
	}

	// check sample rate
	if (m_Driver->canSampleRate((ASIOSampleRate)format.nSamplesPerSec) != ASE_OK)
	{
		rslog::error_ts() << "  unsupported sample rate: " << format.nSamplesPerSec << std::endl;
		return false;
	}

	// check bit depth
	if ((format.wBitsPerSample % 8) != 0)
	{
		rslog::error_ts() << "  bad wBitsPerSample: " << format.wBitsPerSample << std::endl;
		return false;
	}
	const std::vector<ASIOChannelInfo>& channelInfo = output ? m_AsioOutChannelInfo : m_AsioInChannelInfo;

	// make sure all channels use the same format
	ASIOSampleType sampleType;
	if (!CheckSampleTypeAcrossChannels(sampleType, output, firstAsioChannel, (unsigned)actualNumAsioChannels))
	{
		rslog::error_ts() << "  requested channels use multiple different sample types" << std::endl;
		return false;
	}

	// reject unsupported ASIO sample formats
	switch (sampleType)
	{
		case ASIOSTInt16LSB:
		case ASIOSTInt24LSB:
		case ASIOSTInt32LSB:
		case ASIOSTFloat32LSB:
		case ASIOSTFloat64LSB:
			break;
		default:
			rslog::error_ts() << "  ASIO sample type " << sampleType << " is not currently supported" << std::endl;
			return false;
	}

	// block align sanity check
	const WORD expectedBlockAlign = (format.wBitsPerSample / 8) * format.nChannels;
	if (format.nBlockAlign != expectedBlockAlign)
	{
		rslog::error_ts() << "  unexpected nBlockAlign: " << format.nBlockAlign << " | expected: " << expectedBlockAlign << std::endl;
		return false;
	}

	// nAvgBytesPerSec sanity check
	const DWORD expectedBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
	if (format.nAvgBytesPerSec != expectedBytesPerSec)
	{
		rslog::error_ts() << "  unexpected nAvgBytesPerSec: " << format.nAvgBytesPerSec << " | expected: " << expectedBytesPerSec << std::endl;
		return false;
	}

	WORD bitsPerSample = format.wBitsPerSample;
	if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && format.cbSize >= 22)
	{
		const WAVEFORMATEXTENSIBLE& wfe = (const WAVEFORMATEXTENSIBLE&)format;

		// format check
		if (wfe.SubFormat != KSDATAFORMAT_SUBTYPE_PCM && wfe.SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			rslog::info_ts() << "  compressed formats are not supported" << std::endl;
			return false;
		}

		if (wfe.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			if (sampleType != ASIOSTFloat32LSB && sampleType != ASIOSTFloat64LSB)
			{
				rslog::info_ts() << "  rejecting IEEE Float as it's incompatible with current ASIO sample type " << sampleType << std::endl;
				return false;
			}
		}

		// check bit depth
		if (wfe.Format.wBitsPerSample < wfe.Samples.wValidBitsPerSample)
		{
			rslog::info_ts() << "  wBitsPerSample: " << wfe.Format.wBitsPerSample << " is smaller than wValidBitsPerSample: " << wfe.Samples.wValidBitsPerSample << std::endl;
			return false;
		}
		bitsPerSample = wfe.Samples.wValidBitsPerSample;
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
	if (!IsValid() || !m_IsSetup)
		return false;

	long inputLatency = 0;
	long outputLatency = 0;
	if (m_Driver->getLatencies(&inputLatency, &outputLatency) != ASE_OK)
		return false;

	in = AudioFramesToDuration(inputLatency, m_CurrentWaveFormat.Format.nSamplesPerSec);
	out = AudioFramesToDuration(outputLatency, m_CurrentWaveFormat.Format.nSamplesPerSec);

	return true;
}

ASIOBufferInfo* AsioSharedHost::GetOutputBuffer(unsigned channel)
{
	if (!m_IsSetup)
		return nullptr;

	if (channel >= m_AsioOutChannelInfo.size())
		return nullptr;

	return &m_AsioBuffers[channel];
}

ASIOBufferInfo* AsioSharedHost::GetInputBuffer(unsigned channel)
{
	if (!m_IsSetup)
		return nullptr;

	if (channel >= m_AsioInChannelInfo.size())
		return nullptr;

	return &m_AsioBuffers[m_AsioOutChannelInfo.size() + channel];
}

const ASIOChannelInfo* AsioSharedHost::GetOutputChannelInfo(unsigned channel) const
{
	if (channel >= m_AsioOutChannelInfo.size())
		return nullptr;
	return &m_AsioOutChannelInfo[channel];
}

const ASIOChannelInfo* AsioSharedHost::GetInputChannelInfo(unsigned channel) const
{
	if (channel >= m_AsioInChannelInfo.size())
		return nullptr;
	return &m_AsioInChannelInfo[channel];
}

void AsioSharedHost::ResetDebugLogAsioBufferSwitches()
{
	m_dbgNumBufferSwitches = 0;
}

void AsioSharedHost::DisplayCurrentError() const
{
	if (!m_Driver)
		return;

	char err[128] = {};
	m_Driver->getErrorMessage(err);

	rslog::error_ts() << "ASIO Error: " << err << std::endl;
}

void __cdecl AsioSharedHost::AsioCalback_bufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
{
	if (!m_AsioMutex.try_lock())
	{
		rslog::info_ts() << m_DriverName << " " __FUNCTION__ " - aborting buffer switch handling as mutex is already locked." << std::endl;
		return;
	}

	std::lock_guard<std::mutex> guard(m_AsioMutex, std::adopt_lock);

	if (m_StartCount == 0)
		return;

	// disable this later when driver is more mature, for now this logging is important
	if (m_dbgNumBufferSwitches < 2)
	{
		++m_dbgNumBufferSwitches;
		rslog::info_ts() << m_DriverName << " - " __FUNCTION__ " - buffer switch " << m_dbgNumBufferSwitches << std::endl;
	}
	else if (m_dbgNumBufferSwitches == 2)
	{
		++m_dbgNumBufferSwitches;
		rslog::info_ts() << m_DriverName << " - " __FUNCTION__ " - buffer switch " << m_dbgNumBufferSwitches << " (not logging upcoming switches)" << std::endl;
	}

	// zero output
	const size_t numOuts = m_AsioOutChannelInfo.size();
	for (size_t i = 0; i < numOuts; ++i)
	{
		ASIOBufferInfo* asioBuffer = GetOutputBuffer(i);
		if (asioBuffer)
		{
			const UINT32 numBufferBytes = m_NumBufferFrames * GetAsioSampleTypeNumBytes(m_AsioOutChannelInfo[i].type);
			memset(asioBuffer->buffers[doubleBufferIndex], 0, numBufferBytes);
		}
	}

#if TEST_INPUT_NOISE
	const size_t numIns = m_AsioInChannelInfo.size();
	for (size_t i = 0; i < numIns; ++i)
	{
		ASIOBufferInfo* asioBuffer = GetInputBuffer(i);
		if (asioBuffer)
		{
			const UINT32 numBufferBytes = m_NumBufferFrames * GetAsioSampleTypeNumBytes(m_AsioInChannelInfo[i].type);
			BYTE* pBuffer = (BYTE*)asioBuffer->buffers[doubleBufferIndex];
			for (unsigned b = 0; b < numBufferBytes; ++b)
			{
				pBuffer[b] = rand() % 255;
			}
		}
	}
#endif

	for (IAsioBufferSwitchListener* listener : m_AsioBufferListeners)
	{
		listener->OnAsioBufferSwitch((unsigned)doubleBufferIndex);
	}

#if TEST_OUTPUT_NOISE
	for (size_t i = 0; i < numOuts; ++i)
	{
		ASIOBufferInfo* asioBuffer = GetOutputBuffer(i);
		if (asioBuffer)
		{
			const UINT32 numBufferBytes = m_NumBufferFrames * GetAsioSampleTypeNumBytes(m_AsioOutChannelInfo[i].type);
			BYTE* pBuffer = (BYTE*)asioBuffer->buffers[doubleBufferIndex];
			for (unsigned b = 0; b < numBufferBytes; ++b)
			{
				pBuffer[b] = rand() % 255;
			}
		}
	}
#endif

	if (m_PostOutputReady)
		m_Driver->outputReady();
}

void __cdecl AsioSharedHost::AsioCalback_sampleRateDidChange(ASIOSampleRate sRate)
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	if (m_IsSetup)
	{
		// forget about sample rate to restore; something else apparently changed the sample rate
		m_SampleRateRestore.reset();
	}
}

long __cdecl AsioSharedHost::AsioCalback_asioMessage(long selector, long value, void* message, double* opt)
{
	rslog::info_ts() << __FUNCTION__ " - selector: " << selector << " value: " << value << " | returning: ";

	long ret = 0;

	switch (selector)
	{
		case kAsioEngineVersion:
			ret = 1;
			break;
		case kAsioSupportsTimeInfo:
			ret = 0;
			break;
	}

	rslog::info << ret << std::endl;

	return 0;
}

ASIOTime* __cdecl AsioSharedHost::AsioCalback_bufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
{
	AsioCalback_bufferSwitch(doubleBufferIndex, directProcess);

	return nullptr;
}
