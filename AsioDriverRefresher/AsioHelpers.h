#pragma once
#include <vector>
#include <iostream>
#include "stdafx.h"



class AsioSharedHost;

namespace AsioHelpers
{
	struct DriverInfo
	{
		CLSID         Clsid = {};
		std::string   Name;
		std::string   Description;
		std::string   DllPath;
	};

	std::vector<DriverInfo> FindDrivers();
	AsioSharedHost* CreateAsioHost(const DriverInfo& driverInfo);
}
