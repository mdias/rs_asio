#include "stdafx.h"
#include "RSAsioDeviceEnum.h"
#include "RSAsioDevice.h"
#include "AsioHelpers.h"
#include "AsioSharedHost.h"

void RSAsioDeviceEnum::SetConfig(const RSAsioConfig& config)
{
	m_DeviceListNeedsUpdate = true;
	m_Config = config;
}

void RSAsioDeviceEnum::UpdateAvailableDevices()
{
	m_DeviceListNeedsUpdate = false;

	ClearAll();

	std::vector<AsioHelpers::DriverInfo> asioDriversInfo = AsioHelpers::FindDrivers();

	std::vector<AsioSharedHost*> createdAsioHosts;
	auto fnFindOrCreateAsioHost = [&](const std::string& name) -> AsioSharedHost*
	{
		for (auto host : createdAsioHosts)
		{
			char driverName[256];
			host->GetDriver()->getDriverName(driverName);

			if (name == driverName)
			{
				host->AddRef();
				return host;
			}
		}

		for (const auto& info : asioDriversInfo)
		{
			if (info.Name == name)
			{
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
		rslog::info_ts() << __FUNCTION__ " - " << "output requesting ASIO driver: " << m_Config.output.asioDriverName << std::endl;

		AsioSharedHost* host = fnFindOrCreateAsioHost(m_Config.output.asioDriverName);
		if (host)
		{
			RSAsioDevice::Config config;
			config.isOutput = true;
			config.baseAsioChannelNumber = 0;
			config.numAsioChannels = m_Config.output.numChannels;
			config.bufferSizeMode = m_Config.bufferMode;
			config.customBufferSize = m_Config.customBufferSize;
			config.enableSoftwareEndpointVolmeControl = m_Config.output.enableSoftwareEndpointVolumeControl;
			config.enableSoftwareMasterVolumeControl = m_Config.output.enableSoftwareMasterVolumeControl;

			auto device = new RSAsioDevice(*host, L"{ASIO Out}", config);
			device->SetMasterVolumeLevelScalar((float)m_Config.output.softwareMasterVolumePercent / 100.0f);
			m_RenderDevices.AddDevice(device);
			device->Release();
			device = nullptr;

			rslog::info_ts() << __FUNCTION__ " - OK" << std::endl;
			host->Release();
		}
		else
		{
			rslog::error_ts() << __FUNCTION__ " - " << "failed." << std::endl;
		}
	}

	int inputIdx = 0;
	for (const RSAsioInputConfig& inputCfg : m_Config.inputs)
	{
		if (!inputCfg.asioDriverName.empty())
		{
			rslog::info_ts() << __FUNCTION__ " - " << "input[" << inputIdx << "] requesting ASIO driver: " << inputCfg.asioDriverName << std::endl;

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

				auto device = new RSAsioDevice(*host, id, config);
				device->SetMasterVolumeLevelScalar((float)inputCfg.softwareMasterVolumePercent / 100.0f);
				m_CaptureDevices.AddDevice(device);
				device->Release();
				device = nullptr;

				rslog::info_ts() << __FUNCTION__ " - OK" << std::endl;
				host->Release();
			}
			else
			{
				rslog::error_ts() << __FUNCTION__ " - " << "failed." << std::endl;
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
