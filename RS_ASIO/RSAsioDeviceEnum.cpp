#include "stdafx.h"
#include "RSAsioDeviceEnum.h"
#include "RSAsioDevice.h"
#include "AsioHelpers.h"
#include "AsioSharedHost.h"

// ---------------------------------------------------------------------------
// WASAPI device ID → ASIO-backed IMMDevice redirect registry
// Populated at startup when [Asio.InputN] WasapiDevice= is configured.
// ---------------------------------------------------------------------------
struct WasapiRedirectEntry { std::wstring subId; IMMDevice* device; };
static std::vector<WasapiRedirectEntry> s_wasapiRedirects;

void RegisterWasapiRedirect(const std::wstring& wasapiDeviceSubId, IMMDevice* asioDevice)
{
	if (!asioDevice || wasapiDeviceSubId.empty())
		return;
	asioDevice->AddRef();
	// Pre-lowercase so GetWasapiRedirectDevice can compare with a simple find().
	std::wstring lowerSubId = wasapiDeviceSubId;
	std::transform(lowerSubId.begin(), lowerSubId.end(), lowerSubId.begin(), [](wchar_t c) { return (wchar_t)::towlower(c); });
	s_wasapiRedirects.push_back({ lowerSubId, asioDevice });
	rslog::info_ts() << "RegisterWasapiRedirect: " << wasapiDeviceSubId << std::endl;
}

void ClearWasapiRedirects()
{
	for (auto& entry : s_wasapiRedirects)
		entry.device->Release();
	s_wasapiRedirects.clear();
}

IMMDevice* GetWasapiRedirectDevice(const std::wstring& deviceId, const std::wstring& deviceFriendlyName)
{
	if (s_wasapiRedirects.empty())
		return nullptr;

	// Lowercase once before the loop; entries already store pre-lowercased subIds.
	auto toLower = [](std::wstring s) {
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return (wchar_t)::towlower(c); });
		return s;
	};
	const std::wstring lowerDeviceId = toLower(deviceId);
	const std::wstring lowerFriendlyName = toLower(deviceFriendlyName);

	for (auto& entry : s_wasapiRedirects)
	{
		if (lowerDeviceId.find(entry.subId) != std::wstring::npos)
			return entry.device;

		if (!lowerFriendlyName.empty() && lowerFriendlyName.find(entry.subId) != std::wstring::npos)
			return entry.device;
	}
	return nullptr;
}

void RSAsioDeviceEnum::SetConfig(const RSAsioConfig& config)
{
	m_DeviceListNeedsUpdate = true;
	m_Config = config;
}

void RSAsioDeviceEnum::UpdateAvailableDevices()
{
	m_DeviceListNeedsUpdate = false;
	ClearWasapiRedirects();

	ClearAll();

	std::vector<AsioHelpers::DriverInfo> asioDriversInfo = AsioHelpers::FindDrivers();

	std::vector<AsioSharedHost*> createdAsioHosts;
	auto fnFindOrCreateAsioHost = [&](const std::string& name) -> AsioSharedHost*
	{
		for (const auto& info : asioDriversInfo)
		{
			if (info.Name == name)
			{
				// check if we already have this host created
				for (const auto& host : createdAsioHosts)
				{
					if (host->GetAsioDllPath() == info.DllPath)
					{
						host->AddRef();
						return host;
					}
				}

				// if we got here, we don't have it created yet and we'll create it now
				AsioSharedHost* newHost = AsioHelpers::CreateAsioHost(info);
				if (newHost)
				{
					createdAsioHosts.push_back(newHost);
					newHost->AddRef();
					return newHost;
				}
			}
		}

		return nullptr;
	};
	
	if (!m_Config.output.asioDriverName.empty())
	{
		rslog::info_ts() << __FUNCTION__ << " - " << "output requesting ASIO driver: " << m_Config.output.asioDriverName << std::endl;

		AsioSharedHost* host = fnFindOrCreateAsioHost(m_Config.output.asioDriverName);
		if (host)
		{
			RSAsioDevice::Config config;
			config.isOutput = true;
			config.baseAsioChannelNumber = m_Config.output.baseChannel;
			config.altOutputBaseAsioChannelNumber = m_Config.output.altBaseChannel;
			config.numAsioChannels = m_Config.output.numChannels;
			config.bufferSizeMode = m_Config.bufferMode;
			config.customBufferSize = m_Config.customBufferSize;
			config.enableSoftwareEndpointVolmeControl = m_Config.output.enableSoftwareEndpointVolumeControl;
			config.enableSoftwareMasterVolumeControl = m_Config.output.enableSoftwareMasterVolumeControl;
			config.enableRefCountHack = m_Config.output.enableRefCountHack;

			auto device = new RSAsioDevice(*host, L"{ASIO Out}", config);
			device->SetMasterVolumeLevelScalar((float)m_Config.output.softwareMasterVolumePercent / 100.0f);
			m_RenderDevices.AddDevice(device);
			device->Release();
			device = nullptr;

			rslog::info_ts() << __FUNCTION__ << " - OK" << std::endl;
			host->Release();
		}
		else
		{
			rslog::error_ts() << __FUNCTION__ << " - " << "failed." << std::endl;
		}
	}

	int inputIdx = 0;
	for (const RSAsioInputConfig& inputCfg : m_Config.inputs)
	{
		if (!inputCfg.asioDriverName.empty())
		{
			rslog::info_ts() << __FUNCTION__ << " - " << "input[" << inputIdx << "] requesting ASIO driver: " << inputCfg.asioDriverName << std::endl;

			AsioSharedHost* host = fnFindOrCreateAsioHost(inputCfg.asioDriverName);
			if (host)
			{
				wchar_t id[64]{};
				swprintf(id, 63, L"{ASIO IN %i}", inputIdx);

				RSAsioDevice::Config config;
				config.isOutput = false;
				config.baseAsioChannelNumber = inputCfg.useChannel;
				config.numAsioChannels = 1;
				config.bufferSizeMode = m_Config.bufferMode;
				config.customBufferSize = m_Config.customBufferSize;
				config.enableSoftwareEndpointVolmeControl = inputCfg.enableSoftwareEndpointVolumeControl;
				config.enableSoftwareMasterVolumeControl = inputCfg.enableSoftwareMasterVolumeControl;
				config.isMicrophone = inputCfg.microphone;
				config.enableRefCountHack = inputCfg.enableRefCountHack;

				auto device = new RSAsioDevice(*host, id, config);
				device->SetMasterVolumeLevelScalar((float)inputCfg.softwareMasterVolumePercent / 100.0f);

				// Register WASAPI redirect if configured
				if (!inputCfg.wasapiRedirectId.empty())
				{
					const std::wstring wRedirectId(inputCfg.wasapiRedirectId.begin(), inputCfg.wasapiRedirectId.end());
					RegisterWasapiRedirect(wRedirectId, device);
				}

				m_CaptureDevices.AddDevice(device);
				device->Release();
				device = nullptr;

				rslog::info_ts() << __FUNCTION__ << " - OK" << std::endl;
				host->Release();
			}
			else
			{
				rslog::error_ts() << __FUNCTION__ << " - " << "failed." << std::endl;
			}
		}

		++inputIdx;
	}

	// clear up
	for (auto host : createdAsioHosts)
		host->Release();
	createdAsioHosts.clear();

	// fill defaults
	for (IMMDevice* dev : m_RenderDevices)
	{
		for (IMMDevice*& defaultDev : m_DefaultRenderDevices)
		{
			defaultDev = dev;
			defaultDev->AddRef();
		}
		break;
	}
	for (IMMDevice* dev : m_CaptureDevices)
	{
		for (IMMDevice*& defaultDev : m_DefaultCaptureDevices)
		{
			defaultDev = dev;
			defaultDev->AddRef();
		}
		break;
	}
}
