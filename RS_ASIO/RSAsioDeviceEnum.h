#pragma once

#include "RSBaseDeviceEnum.h"
#include "AsioSharedHost.h"

struct RSAsioOutputConfig
{
	std::string asioDriverName;
	unsigned numChannels = 2;
};

struct RSAsioInputConfig
{
	std::string asioDriverName;
	unsigned useChannel = 0;
};

struct RSAsioConfig
{
	RSAsioOutputConfig output;
	std::array<RSAsioInputConfig, 2> inputs;
	AsioSharedHost::BufferSizeMode bufferMode = AsioSharedHost::BufferSizeMode_Driver;
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
