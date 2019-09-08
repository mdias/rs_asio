#pragma once

class AsioSharedHost;

namespace AsioHelpers
{
	struct DriverInfo
	{
		CLSID         Clsid = {};
		std::string   Name;
		std::string   DllPath;
	};

	std::vector<DriverInfo> FindDrivers();
	AsioSharedHost* CreateAsioHost(const DriverInfo& driverInfo);
}
