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
		std::cout << __FUNCTION__ " - " << "output requesting ASIO driver: " << m_Config.output.asioDriverName << "\n";

		AsioSharedHost* host = fnFindOrCreateAsioHost(m_Config.output.asioDriverName);
		if (host)
		{
			auto device = new RSAsioDevice(*host, L"{ASIO Out}", true, 0, m_Config.output.numChannels);
			m_RenderDevices.AddDevice(device);
			device->Release();
			device = nullptr;

			std::cout << __FUNCTION__ " - OK\n";
			host->Release();
		}
		else
		{
			std::cerr << __FUNCTION__ " - " << "failed.\n";
		}
	}

	int inputIdx = 0;
	for (const RSAsioInputConfig& inputCfg : m_Config.inputs)
	{
		if (!inputCfg.asioDriverName.empty())
		{
			std::cout << __FUNCTION__ " - " << "input[" << inputIdx << "] requesting ASIO driver: " << inputCfg.asioDriverName << "\n";

			AsioSharedHost* host = fnFindOrCreateAsioHost(inputCfg.asioDriverName);
			if (host)
			{
				wchar_t id[64]{};
				swprintf(id, 63, L"{ASIO IN %i}", inputIdx);

				auto device = new RSAsioDevice(*host, id, false, inputCfg.useChannel, 1);
				m_CaptureDevices.AddDevice(device);
				device->Release();
				device = nullptr;

				std::cout << __FUNCTION__ " - OK\n";
				host->Release();
			}
			else
			{
				std::cerr << __FUNCTION__ " - " << "failed.\n";
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
