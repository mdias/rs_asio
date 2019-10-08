#pragma once

#include "RSBaseDeviceEnum.h"
#include "RSAsioDevice.h"

struct RSAsioOutputConfig
{
	std::string asioDriverName;
	unsigned numChannels = 2;
	bool enableSoftwareEndpointVolumeControl = false;
	bool enableSoftwareMasterVolumeControl = false;
	int softwareMasterVolumePercent = 100;
};

struct RSAsioInputConfig
{
	std::string asioDriverName;
	unsigned useChannel = 0;
	bool enableSoftwareEndpointVolumeControl = false;
	bool enableSoftwareMasterVolumeControl = false;
	int softwareMasterVolumePercent = 100;
};

struct RSAsioConfig
{
	RSAsioOutputConfig output;
	std::array<RSAsioInputConfig, 2> inputs;
	RSAsioDevice::BufferSizeMode bufferMode = RSAsioDevice::BufferSizeMode_Driver;
	unsigned customBufferSize = 128;
};

struct RSConfig
{
	bool enableWasapi = false;
	bool enableAsio = false;
	RSAsioConfig asioConfig;
};

class RSAsioDeviceEnum : public RSBaseDeviceEnum
{
public:
	void SetConfig(const RSAsioConfig& config);

protected:
	virtual void UpdateAvailableDevices() override;

	RSAsioConfig m_Config;
};
