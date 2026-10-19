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
#include <mmdeviceapi.h>
#include <combaseapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <ksmedia.h>

#ifdef __GNUC__
#include "FunctionDiscoveryKeys_devpkey.h"
#ifndef __IAudioClient3_INTERFACE_DEFINED__
#define __IAudioClient3_INTERFACE_DEFINED__
DEFINE_GUID(IID_IAudioClient3, 0x7ed4ee07, 0x8e67, 0x4cd4, 0x8c,0x1a, 0x2b,0x7a,0x59,0x87,0xad,0x42);
MIDL_INTERFACE("7ed4ee07-8e67-4cd4-8c1a-2b7a5987ad42")
IAudioClient3 : public IAudioClient2
{
    virtual HRESULT STDMETHODCALLTYPE GetSharedModeEnginePeriod(
        const WAVEFORMATEX *pFormat,
        UINT32 *pDefaultPeriodInFrames,
        UINT32 *pFundamentalPeriodInFrames,
        UINT32 *pMinPeriodInFrames,
        UINT32 *pMaxPeriodInFrames) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSharedModeEnginePeriod(
        WAVEFORMATEX **ppFormat,
        UINT32 *pCurrentPeriodInFrames) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitializeSharedAudioStream(
        DWORD StreamFlags,
        UINT32 PeriodInFrames,
        const WAVEFORMATEX *pFormat,
        LPCGUID AudioSessionGuid) = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IAudioClient3, 0x7ed4ee07, 0x8e67, 0x4cd4, 0x8c,0x1a, 0x2b,0x7a,0x59,0x87,0xad,0x42)
#endif
#endif
#else
#include <functiondiscoverykeys_devpkey.h>
#endif


// reference additional headers your program requires here
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <array>
#include <set>
#include <string>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <functional>
#include <mutex>
#include <optional>

#include "asio.h"
#include "Utils.h"
#include "Log.h"
#include "TrampolineToMethod.h"
