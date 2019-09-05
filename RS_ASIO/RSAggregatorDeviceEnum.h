#pragma once

#include "RSBaseDeviceEnum.h"

class RSAggregatorDeviceEnum : public RSBaseDeviceEnum
{
public:
	RSAggregatorDeviceEnum();
	RSAggregatorDeviceEnum(const RSAggregatorDeviceEnum&) = delete;
	RSAggregatorDeviceEnum(RSAggregatorDeviceEnum&&) = delete;
	virtual ~RSAggregatorDeviceEnum();

	void AddDeviceEnumerator(IMMDeviceEnumerator* enumerator);

protected:
	virtual void UpdateAvailableDevices() override;
	virtual void ClearAll() override;

	std::set<IMMDeviceEnumerator*> m_Enumerators;
};
