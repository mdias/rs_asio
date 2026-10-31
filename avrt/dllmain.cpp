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
#ifdef _MSC_VER
static REAL_FUNCTIONS RealFunctions = {};
#else
extern "C" { REAL_FUNCTIONS RealFunctions = {}; }
#endif

#ifdef _MSC_VER
#define IMPLEMENT_FORWARDER(x, idx) void __declspec(naked) Wrapped_##x(){ _asm { jmp RealFunctions.##x }; }
#else
// GCC/Clang (mingw): emit the tail-jump in a top-level asm block. "idx" is the
// position of the function pointer inside REAL_FUNCTIONS.
#define ASM_STR2(x) #x
#define ASM_STR(x) ASM_STR2(x)
#define ASM_SYM(name) ASM_STR(__USER_LABEL_PREFIX__) name
#ifdef _WIN64
#define ASM_JMP(idx) "jmp *(" ASM_SYM("RealFunctions") " + " ASM_STR(idx) "*8)(%rip)"
#else
#define ASM_JMP(idx) "jmp *(" ASM_SYM("RealFunctions") " + " ASM_STR(idx) "*4)"
#endif
#define IMPLEMENT_FORWARDER(x, idx) \
	extern "C" void Wrapped_##x(); \
	asm(".text\n" \
		".globl " ASM_SYM("Wrapped_" #x) "\n" \
		ASM_SYM("Wrapped_" #x) ":\n\t" \
		ASM_JMP(idx) "\n");
#endif
#define FILLREALPROCADDRESS(x) RealFunctions.x = (decltype(RealFunctions.x))GetProcAddress(hModuleReal, #x);

IMPLEMENT_FORWARDER(AvCreateTaskIndex, 0);
IMPLEMENT_FORWARDER(AvQuerySystemResponsiveness, 1);
IMPLEMENT_FORWARDER(AvQueryTaskIndexValue, 2);
IMPLEMENT_FORWARDER(AvRevertMmThreadCharacteristics, 3);
IMPLEMENT_FORWARDER(AvRtCreateThreadOrderingGroup, 4);
IMPLEMENT_FORWARDER(AvRtCreateThreadOrderingGroupExA, 5);
IMPLEMENT_FORWARDER(AvRtCreateThreadOrderingGroupExW, 6);
IMPLEMENT_FORWARDER(AvRtDeleteThreadOrderingGroup, 7);
IMPLEMENT_FORWARDER(AvRtJoinThreadOrderingGroup, 8);
IMPLEMENT_FORWARDER(AvRtLeaveThreadOrderingGroup, 9);
IMPLEMENT_FORWARDER(AvRtWaitOnThreadOrderingGroup, 10);
IMPLEMENT_FORWARDER(AvSetMmMaxThreadCharacteristicsA, 11);
IMPLEMENT_FORWARDER(AvSetMmMaxThreadCharacteristicsW, 12);
IMPLEMENT_FORWARDER(AvSetMmThreadCharacteristicsA, 13);
IMPLEMENT_FORWARDER(AvSetMmThreadCharacteristicsW, 14);
IMPLEMENT_FORWARDER(AvSetMmThreadPriority, 15);
IMPLEMENT_FORWARDER(AvSetMultimediaMode, 16);
IMPLEMENT_FORWARDER(AvTaskIndexYield, 17);
IMPLEMENT_FORWARDER(AvTaskIndexYieldCancel, 18);
IMPLEMENT_FORWARDER(AvThreadOpenTaskIndex, 19);

static void LoadOriginalDll()
{
	char sysDir[MAX_PATH] = {};
	GetSystemDirectoryA(sysDir, MAX_PATH);

	char originalPath[MAX_PATH] = {};
	snprintf(originalPath, sizeof(originalPath), "%s\\avrt.dll", sysDir);

	std::cout << "Attempting to load original DLL from: " << originalPath << std::endl;
	hModuleReal = LoadLibraryA(originalPath);
	if (!hModuleReal)
	{
		std::cerr << "Load failed" << std::endl;
	}
	else
	{
		std::cout << "Load OK; fetching procedure addresses..." << std::endl;

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
		std::cout << "Unloading original DLL" << std::endl;

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

static HWND GetGameWindow()
{
	return FindWindowA("Rocksmith 2014", "Rocksmith 2014");
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
		// InitConsole();
#endif
		LoadOriginalDll();
		if (!LoadLibrary(TEXT("RS_ASIO.dll")))
		{
			MessageBox(GetGameWindow(), TEXT("Failed to load RS_ASIO.dll"), TEXT("Error"), MB_OK | MB_ICONERROR);
			exit(1);
		}
		
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

