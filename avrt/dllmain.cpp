// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

typedef int(*BLINDWRAPPER)(void);

struct REAL_FUNCTIONS
{
	BLINDWRAPPER AvCreateTaskIndex;
	BLINDWRAPPER AvQuerySystemResponsiveness;
	BLINDWRAPPER AvQueryTaskIndexValue;
	BLINDWRAPPER AvRevertMmThreadCharacteristics;
	BLINDWRAPPER AvRtCreateThreadOrderingGroup;
	BLINDWRAPPER AvRtCreateThreadOrderingGroupExA;
	BLINDWRAPPER AvRtCreateThreadOrderingGroupExW;
	BLINDWRAPPER AvRtDeleteThreadOrderingGroup;
	BLINDWRAPPER AvRtJoinThreadOrderingGroup;
	BLINDWRAPPER AvRtLeaveThreadOrderingGroup;
	BLINDWRAPPER AvRtWaitOnThreadOrderingGroup;
	BLINDWRAPPER AvSetMmMaxThreadCharacteristicsA;
	BLINDWRAPPER AvSetMmMaxThreadCharacteristicsW;
	BLINDWRAPPER AvSetMmThreadCharacteristicsA;
	BLINDWRAPPER AvSetMmThreadCharacteristicsW;
	BLINDWRAPPER AvSetMmThreadPriority;
	BLINDWRAPPER AvSetMultimediaMode;
	BLINDWRAPPER AvTaskIndexYield;
	BLINDWRAPPER AvTaskIndexYieldCancel;
	BLINDWRAPPER AvThreadOpenTaskIndex;
};

static HMODULE hModuleReal = NULL;
static REAL_FUNCTIONS RealFunctions = {};

#define IMPLEMENT_FORWARDER(x) void __declspec(naked) Wrapped_##x(){ _asm { jmp RealFunctions.##x }; }
#define FILLREALPROCADDRESS(x) RealFunctions.##x = (decltype(RealFunctions.##x))GetProcAddress(hModuleReal, #x);

IMPLEMENT_FORWARDER(AvCreateTaskIndex);
IMPLEMENT_FORWARDER(AvQuerySystemResponsiveness);
IMPLEMENT_FORWARDER(AvQueryTaskIndexValue);
IMPLEMENT_FORWARDER(AvRevertMmThreadCharacteristics);
IMPLEMENT_FORWARDER(AvRtCreateThreadOrderingGroup);
IMPLEMENT_FORWARDER(AvRtCreateThreadOrderingGroupExA);
IMPLEMENT_FORWARDER(AvRtCreateThreadOrderingGroupExW);
IMPLEMENT_FORWARDER(AvRtDeleteThreadOrderingGroup);
IMPLEMENT_FORWARDER(AvRtJoinThreadOrderingGroup);
IMPLEMENT_FORWARDER(AvRtLeaveThreadOrderingGroup);
IMPLEMENT_FORWARDER(AvRtWaitOnThreadOrderingGroup);
IMPLEMENT_FORWARDER(AvSetMmMaxThreadCharacteristicsA);
IMPLEMENT_FORWARDER(AvSetMmMaxThreadCharacteristicsW);
IMPLEMENT_FORWARDER(AvSetMmThreadCharacteristicsA);
IMPLEMENT_FORWARDER(AvSetMmThreadCharacteristicsW);
IMPLEMENT_FORWARDER(AvSetMmThreadPriority);
IMPLEMENT_FORWARDER(AvSetMultimediaMode);
IMPLEMENT_FORWARDER(AvTaskIndexYield);
IMPLEMENT_FORWARDER(AvTaskIndexYieldCancel);
IMPLEMENT_FORWARDER(AvThreadOpenTaskIndex);

static void LoadOriginalDll()
{
	char sysDir[MAX_PATH] = {};
	GetSystemDirectoryA(sysDir, MAX_PATH);

	char originalPath[MAX_PATH] = {};
	snprintf(originalPath, sizeof(originalPath), "%s\\avrt.dll", sysDir);

	std::cout << "Attempting to load original DLL from: " << originalPath << "\n";
	hModuleReal = LoadLibraryA(originalPath);
	if (!hModuleReal)
	{
		std::cerr << "Load failed\n";
	}
	else
	{
		std::cout << "Load OK; fetching procedure addresses...\n";

		FILLREALPROCADDRESS(AvCreateTaskIndex);
		FILLREALPROCADDRESS(AvQuerySystemResponsiveness);
		FILLREALPROCADDRESS(AvQueryTaskIndexValue);
		FILLREALPROCADDRESS(AvRevertMmThreadCharacteristics);
		FILLREALPROCADDRESS(AvRtCreateThreadOrderingGroup);
		FILLREALPROCADDRESS(AvRtCreateThreadOrderingGroupExA);
		FILLREALPROCADDRESS(AvRtCreateThreadOrderingGroupExW);
		FILLREALPROCADDRESS(AvRtDeleteThreadOrderingGroup);
		FILLREALPROCADDRESS(AvRtJoinThreadOrderingGroup);
		FILLREALPROCADDRESS(AvRtLeaveThreadOrderingGroup);
		FILLREALPROCADDRESS(AvRtWaitOnThreadOrderingGroup);
		FILLREALPROCADDRESS(AvSetMmMaxThreadCharacteristicsA);
		FILLREALPROCADDRESS(AvSetMmMaxThreadCharacteristicsW);
		FILLREALPROCADDRESS(AvSetMmThreadCharacteristicsA);
		FILLREALPROCADDRESS(AvSetMmThreadCharacteristicsW);
		FILLREALPROCADDRESS(AvSetMmThreadPriority);
		FILLREALPROCADDRESS(AvSetMultimediaMode);
		FILLREALPROCADDRESS(AvTaskIndexYield);
		FILLREALPROCADDRESS(AvTaskIndexYieldCancel);
		FILLREALPROCADDRESS(AvThreadOpenTaskIndex);
	}
}

static void UnloadOriginalDll()
{
	if (hModuleReal)
	{
		std::cout << "Unloading original DLL\n";

		FreeLibrary(hModuleReal);
		hModuleReal = NULL;
	}
}

static void InitConsole()
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());

	FILE* fDummy = nullptr;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		InitConsole();
#endif
		LoadOriginalDll();
		LoadLibrary(TEXT("RS_ASIO.dll"));
		
		break;
	case DLL_PROCESS_DETACH:
		UnloadOriginalDll();
		break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
    }
    return TRUE;
}

