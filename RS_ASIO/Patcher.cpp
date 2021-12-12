#include "stdafx.h"
#include "dllmain.h"

typedef HRESULT(STDAPICALLTYPE *CoCreateInstancePtr)(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void **ppOut);

static const BYTE originalBytes_call_CoCreateInstance[]{
	0xff, 0x15, 0x34, 0x87, 0x18, 0x02,
	0x85, 0xc0
};

static const BYTE originalBytes_call_PortAudio_MarshalStreamComPointers[]{
	0xe8, 0x9e, 0xe1, 0xff, 0xff, // call
	0x83, 0xc4, 0x04, // add esp, 4
	0x85, 0xc0 // test eax, eax
};

static const BYTE originalBytes_call_UnmarshalStreamComPointers[]{
	0xe8, 0x59, 0xdf, 0xff, 0xff, // call
	0x57, // push edi
	0xe8, 0x33, 0xe0, 0xff, 0xff // call (to another uninteresting function)
};

static const uintptr_t location_TwoRealToneCablesMessageBox = 0x017b9518;

static const BYTE originalBytes_TwoRealToneCablesMessageBox[]{
	0x8b, 0xb5, 0x8c, 0xff, 0xff, 0xff // mov esi, dword ptr [ebp-0x74]
};

static const BYTE patchedBytes_TwoRealToneCablesMessageBox[]{
	0xe9, 0x2f, 0x01, 0x00, 0x00, // jmp to 0x17b964c (skip over the message box)
	0x90 // nop
};

static std::vector<void*> FindBytesOffsets(const BYTE* bytes, size_t numBytes)
{
	std::vector<void*> result;

	const HMODULE baseModuleHandle = GetModuleHandle(NULL);
	MODULEINFO baseModuleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), baseModuleHandle, &baseModuleInfo, sizeof(baseModuleInfo)))
	{
		rslog::error_ts() << "Could not get base module info" << std::endl;
		return result;
	}

	BYTE* addr = (BYTE*)baseModuleInfo.lpBaseOfDll;
	const DWORD maxSearchAddr = baseModuleInfo.SizeOfImage - numBytes;

	for (DWORD offset = 0; offset < maxSearchAddr; ++offset)
	{
		bool match = true;
		for (DWORD i = 0; i < numBytes; ++i)
		{
			if (addr[offset + i] != bytes[i])
			{
				match = false;
				break;
			}
		}

		if (match)
		{
			result.push_back((void*)(addr + offset));
		}
	}

	return result;
}

/// <summary>
/// Write x86 ASM (HEX) to static address.
/// </summary>
/// <param name="location"> - Pointer you want to edit</param>
/// <param name="newAssembly"> - Edit you want to make</param>
/// <param name="lengthOfAssembly"> - How long is the edit</param>
/// <returns>Patch Completed</returns>
static bool Patch_ReplaceAssembly(LPVOID location, LPVOID newAssembly, UINT lengthOfAssembly) {
	DWORD dwOldProt, dwDummy;

	if (!VirtualProtect(location, lengthOfAssembly, PAGE_EXECUTE_READWRITE, &dwOldProt)) {
		return false;
	}

	memcpy(location, newAssembly, lengthOfAssembly);

	FlushInstructionCache(GetCurrentProcess(), location, lengthOfAssembly);
	VirtualProtect(location, lengthOfAssembly, dwOldProt, &dwDummy);

	return true;
}

template<void* NewFn>
static void Patch_CallAbsoluteAddress(const std::vector<void*>& offsets)
{
	rslog::info_ts() << __FUNCTION__ " - num locations: " << offsets.size() << std::endl;

	static void* fnAddress = NewFn;
	static void* fnAddressIndirect = &fnAddress;

	for (void* offset : offsets)
	{
		rslog::info_ts() << "Patching call at " << offset << std::endl;

		BYTE* bytes = (BYTE*)offset;

		DWORD oldProtectFlags = 0;
		if (!VirtualProtect(offset, 6, PAGE_WRITECOPY, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to change memory protection" << std::endl;
		}
		else
		{
			void** callAddress = (void**)(bytes + 2);
			*callAddress = fnAddressIndirect;

			if (!VirtualProtect(offset, 6, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

template<void* NewFn>
static void Patch_CallRelativeAddress(const std::vector<void*>& offsets)
{
	rslog::info_ts() << __FUNCTION__ " - num locations: " << offsets.size() << std::endl;

	static void* fnAddress = NewFn;
	static void* fnAddressIndirect = &fnAddress;

	for (void* offset : offsets)
	{
		rslog::info_ts() << "Patching call at " << offset << std::endl;

		BYTE* bytes = (BYTE*)offset;

		std::int32_t relOffset = *(std::int32_t*)(bytes + 1);
		bytes += 5 + relOffset;

		DWORD oldProtectFlags = 0;
		if (!VirtualProtect(offset, 6, PAGE_WRITECOPY, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to change memory protection" << std::endl;
		}
		else
		{
			// insert jump function
			bytes[0] = 0xff;
			bytes[1] = 0x25;
			void** callAddress = (void**)(bytes + 2);
			*callAddress = fnAddressIndirect;

			if (!VirtualProtect(offset, 6, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}


void PatchOriginalCode()
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	std::vector<void*> offsets_CoCreateInstance = FindBytesOffsets(originalBytes_call_CoCreateInstance, sizeof(originalBytes_call_CoCreateInstance));
	std::vector<void*> offsets_PaMarshalPointers = FindBytesOffsets(originalBytes_call_PortAudio_MarshalStreamComPointers, sizeof(originalBytes_call_PortAudio_MarshalStreamComPointers));
	std::vector<void*> offsets_PaUnmarshalPointers = FindBytesOffsets(originalBytes_call_UnmarshalStreamComPointers, sizeof(originalBytes_call_UnmarshalStreamComPointers));

	if (offsets_CoCreateInstance.size() == 0 && offsets_PaMarshalPointers.size() == 0 && offsets_PaUnmarshalPointers.size() == 0)
	{
		rslog::error_ts() << "No valid locations for patching were found. Make sure you're trying this on the right game version." << std::endl;
	}
	else
	{
		// patch CoCreateInstance calls
		rslog::info_ts() << "Patching CoCreateInstance" << std::endl;
		Patch_CallAbsoluteAddress<(void*)&Patched_CoCreateInstance>(offsets_CoCreateInstance);

		// patch PortAudio MarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio MarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress<(void*)&Patched_PortAudio_MarshalStreamComPointers>(offsets_PaMarshalPointers);

		// patch PortAudio UnmarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio UnmarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress<(void*)&Patched_PortAudio_UnmarshalStreamComPointers>(offsets_PaUnmarshalPointers);

		Patch_ReplaceAssembly((LPVOID)location_TwoRealToneCablesMessageBox, (LPVOID)patchedBytes_TwoRealToneCablesMessageBox, sizeof(patchedBytes_TwoRealToneCablesMessageBox));
	}
}