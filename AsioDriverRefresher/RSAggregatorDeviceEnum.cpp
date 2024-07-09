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

void RSAggregatorDeviceEnum::AddDeviceEnumerator(IMMDeviceEnumerator* enumerator, bool enableOutputs, bool enableInputs)
{
	if (!enumerator)
		return;

	TrackedEnumerator* existingTrackedEnum = FindTrackedEnumerator(enumerator);
	if (!existingTrackedEnum)
	{
		m_Enumerators.emplace_back(enumerator, enableOutputs, enableInputs);
	}
	else
	{
		existingTrackedEnum->enableOutputs |= enableOutputs;
		existingTrackedEnum->enableInputs |= enableInputs;
	}

	m_DeviceListNeedsUpdate = true;
}

RSAggregatorDeviceEnum::TrackedEnumerator* RSAggregatorDeviceEnum::FindTrackedEnumerator(IMMDeviceEnumerator* enumerator)
{
	for (RSAggregatorDeviceEnum::TrackedEnumerator& te : m_Enumerators)
	{
		if (te.enumerator == enumerator)
			return &te;
	}
	return nullptr;
}

void RSAggregatorDeviceEnum::UpdateAvailableDevices()
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

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

	for (const RSAggregatorDeviceEnum::TrackedEnumerator& te : m_Enumerators)
	{
		if (te.enableOutputs)
			fnUpdateCollection(te.enumerator, eRender, aggregatedRenderCollection, m_DefaultRenderDevices);
		if (te.enableInputs)
			fnUpdateCollection(te.enumerator, eCapture, aggregatedCaptureCollection, m_DefaultCaptureDevices);
	}

	m_RenderDevices.UpdateDevicesFromCollection(aggregatedRenderCollection, true);
	m_CaptureDevices.UpdateDevicesFromCollection(aggregatedCaptureCollection, true);

	m_DeviceListNeedsUpdate = false;
}

void RSAggregatorDeviceEnum::ClearAll()
{
	RSBaseDeviceEnum::ClearAll();
	m_Enumerators.clear();
}