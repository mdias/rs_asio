// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <psapi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <combaseapi.h>
#include <Audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <cmath>



// reference additional headers your program requires here
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <array>
#include <set>
#include <string>
#include <cctype>
#include <algorithm>
#include <functional>
#include <mutex>
#include <optional>
#include "asio.h"
#include "Utils.h"
#include "Log.h"
#include "FileHelper.h"
#include "Config.h"