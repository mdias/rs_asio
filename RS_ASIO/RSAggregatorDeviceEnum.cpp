// RSDeviceEnumWrapper.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "RSAggregatorDeviceEnum.h"

RSAggregatorDeviceEnum::RSAggregatorDeviceEnum()
{
}

RSAggregatorDeviceEnum::~RSAggregatorDeviceEnum()
{
	ClearAll();
}

void RSAggregatorDeviceEnum::AddDeviceEnumerator(IMMDeviceEnumerator* enumerator)
{
	if (!enumerator)
		return;

	auto it = m_Enumerators.find(enumerator);
	if (it == m_Enumerators.end())
	{
		m_Enumerators.emplace(enumerator);
		enumerator->AddRef();
	}

	m_DeviceListNeedsUpdate = true;
}

void RSAggregatorDeviceEnum::UpdateAvailableDevices()
{
	std::cout << __FUNCTION__ "\n";

	RSDeviceCollection aggregatedRenderCollection;
	RSDeviceCollection aggregatedCaptureCollection;

	// release defaults
	for (IMMDevice*& dev : m_DefaultRenderDevices)
	{
		if (dev)
		{
			dev->Release();
			dev = nullptr;
		}
	}
	for (IMMDevice*& dev : m_DefaultCaptureDevices)
	{
		if (dev)
		{
			dev->Release();
			dev = nullptr;
		}
	}

	auto fnUpdateCollection = [](IMMDeviceEnumerator* enumerator, EDataFlow dataFlow, RSDeviceCollection& out, std::array<IMMDevice*, ERole_enum_count>& outDefaults)
	{
		IMMDeviceCollection* collection = nullptr;
		if (SUCCEEDED(enumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATEMASK_ALL, &collection)))
		{
			out.UpdateDevicesFromCollection(*collection, false);
			collection->Release();
		}

		for (int i = eConsole; i < ERole_enum_count; ++i)
		{
			const ERole role = (ERole)i;
			if (!outDefaults[role])
			{
				IMMDevice* defaultDevice = nullptr;

				if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(dataFlow, role, &defaultDevice)))
				{
					outDefaults[role] = defaultDevice;
				}
			}
		}
	};

	for (IMMDeviceEnumerator* e : m_Enumerators)
	{
		fnUpdateCollection(e, eRender, aggregatedRenderCollection, m_DefaultRenderDevices);
		fnUpdateCollection(e, eCapture, aggregatedCaptureCollection, m_DefaultCaptureDevices);
	}

	m_RenderDevices.UpdateDevicesFromCollection(aggregatedRenderCollection, true);
	m_CaptureDevices.UpdateDevicesFromCollection(aggregatedCaptureCollection, true);

	m_DeviceListNeedsUpdate = false;
}

void RSAggregatorDeviceEnum::ClearAll()
{
	RSBaseDeviceEnum::ClearAll();

	for (IMMDeviceEnumerator* enumerator : m_Enumerators)
	{
		if (enumerator)
			enumerator->Release();
	}
	m_Enumerators.clear();
}