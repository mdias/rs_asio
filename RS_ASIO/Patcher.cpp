#include "stdafx.h"
#include "dllmain.h"

typedef HRESULT(STDAPICALLTYPE *CoCreateInstancePtr)(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void **ppOut);

static const BYTE originalBytes_call_CoCreateInstance[]{
	0xe8, 0x3e, 0x9d, 0x7d, 0x00, // call relative
	0xcd, // ?
	0x85, 0xc0 // test eax, eax
};

static const BYTE originalBytes_call_CoCreateInstance2[]{
	0xe8, 0xc2, 0x23, 0x57, 0x00, // call relative
	0x96, // xchg eax, esi
	0x85, 0xc0 // test eax, eax
};

// these are unused for now
/*
static const BYTE originalBytes_call_CoCreateInstance3[]{
	0xe8, 0x4d, 0xfa, 0x5a, 0x00, // call relative
	0x85, 0xc0, // test eax, eax
	0x75, 0x74 // jnz short ..
};

static const BYTE originalBytes_call_CoCreateInstance4[]{
	0xe8, 0x31, 0xdd, 0x4a, 0x00, // call relative
	0x85, 0xc0, // test eax, eax
	0x79, 0x0c // jns short ..
};

static const BYTE originalBytes_call_CoCreateInstance5[]{
	0xe8, 0x79, 0x7a, 0x4a, 0x00, // call relative
	0x85, 0xc0, // test eax, eax
	0x78, 0x33 // js short ..
};
*/


static const BYTE originalBytes_call_PortAudio_MarshalStreamComPointers[]{
	0xe8, 0xdc, 0xe4, 0xff, 0xff, // call
	0x83, 0xc4, 0x04, // add esp, 4
	0x85, 0xc0 // test eax, eax
};

static const BYTE originalBytes_call_UnmarshalStreamComPointers[]{
	0xe8, 0x97, 0xe2, 0xff, 0xff, // call
	0x57, // push edi
	0xe8, 0x71, 0xe3, 0xff, 0xff // call (to another uninteresting function)
};

static const BYTE originalBytes_TwoRealToneCablesMessageBox[]{
//	0x8b, 0x75, 0x8c, // mov esi, ....
	0xe8, 0x7c, 0x22, 0xe2, 0xff, // call
	0x84, 0xc0, // test al, al
	0x74, 0x6b, // jz ...
	0xba, 0x34, 0xfe, 0x1c, 0x01, // mov edx, offset "TooManyGuitarInputs"
	0x8d, 0xb3, 0x4c, 0x01, 0x00, 0x00, // lea esi, [ebx+0x14C]
	0xe8, 0xd8, 0xd8, 0xc4, 0xff // call
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

static void Patch_ReplaceWithNops(void* offset, size_t numBytes)
{
	DWORD oldProtectFlags = 0;
	if (!VirtualProtect(offset, numBytes, PAGE_WRITECOPY, &oldProtectFlags))
	{
		rslog::error_ts() << "Failed to change memory protection" << std::endl;
	}
	else
	{
		BYTE* byte = (BYTE*)offset;
		for (size_t i = 0; i < numBytes; ++i)
		{
			byte[i] = 0x90; // nop
		}

		FlushInstructionCache(GetCurrentProcess(), offset, numBytes);
		if (!VirtualProtect(offset, numBytes, oldProtectFlags, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to restore memory protection" << std::endl;
		}
	}
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
			bytes[0] = 0xff;
			bytes[1] = 0x15;

			void** callAddress = (void**)(bytes + 2);
			*callAddress = fnAddressIndirect;

			FlushInstructionCache(GetCurrentProcess(), offset, 6);
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
		if (!VirtualProtect(bytes, 6, PAGE_WRITECOPY, &oldProtectFlags))
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

			FlushInstructionCache(GetCurrentProcess(), bytes, 6);
			if (!VirtualProtect(bytes, 6, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

template<typename T>
void vector_append(std::vector<T>& inOut, const std::vector<T>& source)
{
	for (auto& it : source)
	{
		inOut.push_back(it);
	}
}

void PatchOriginalCode()
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	std::vector<void*> offsets_CoCreateInstanceAbs = FindBytesOffsets(originalBytes_call_CoCreateInstance, sizeof(originalBytes_call_CoCreateInstance));
	vector_append(offsets_CoCreateInstanceAbs, FindBytesOffsets(originalBytes_call_CoCreateInstance2, sizeof(originalBytes_call_CoCreateInstance2)));

	// this is not patching properly, and causing issues with midi stuff in RSMods.
	// we don't seem to need it, so let's keep it disabled.
	//std::vector<void*> offsets_CoCreateInstanceRel = FindBytesOffsets(originalBytes_call_CoCreateInstance3, sizeof(originalBytes_call_CoCreateInstance3));
	//vector_append(offsets_CoCreateInstanceRel, FindBytesOffsets(originalBytes_call_CoCreateInstance4, sizeof(originalBytes_call_CoCreateInstance4)));
	//vector_append(offsets_CoCreateInstanceRel, FindBytesOffsets(originalBytes_call_CoCreateInstance5, sizeof(originalBytes_call_CoCreateInstance5)));

	std::vector<void*> offsets_PaMarshalPointers = FindBytesOffsets(originalBytes_call_PortAudio_MarshalStreamComPointers, sizeof(originalBytes_call_PortAudio_MarshalStreamComPointers));
	std::vector<void*> offsets_PaUnmarshalPointers = FindBytesOffsets(originalBytes_call_UnmarshalStreamComPointers, sizeof(originalBytes_call_UnmarshalStreamComPointers));

	std::vector<void*> offsets_TwoRealToneCablesMessageBox = FindBytesOffsets(originalBytes_TwoRealToneCablesMessageBox, sizeof(originalBytes_TwoRealToneCablesMessageBox));

	if (offsets_CoCreateInstanceAbs.size() == 0 && offsets_PaMarshalPointers.size() == 0 && offsets_PaUnmarshalPointers.size() == 0)
	{
		rslog::error_ts() << "No valid locations for patching were found. Make sure you're trying this on the right game version." << std::endl;
	}
	else
	{
		// patch CoCreateInstance calls
		rslog::info_ts() << "Patching CoCreateInstance" << std::endl;
		Patch_CallAbsoluteAddress<(void*)&Patched_CoCreateInstance>(offsets_CoCreateInstanceAbs);
		//Patch_CallRelativeAddress<(void*)&Patched_CoCreateInstance>(offsets_CoCreateInstanceRel);

		// patch PortAudio MarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio MarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress<(void*)&Patched_PortAudio_MarshalStreamComPointers>(offsets_PaMarshalPointers);

		// patch PortAudio UnmarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio UnmarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress<(void*)&Patched_PortAudio_UnmarshalStreamComPointers>(offsets_PaUnmarshalPointers);

		// patch two guitar cables connected message in single-player
		rslog::info_ts() << "Patching Two Guitar Tones Connected Message Box (num locations: " << offsets_TwoRealToneCablesMessageBox.size() << ")" << std::endl;
		for (void* offset : offsets_TwoRealToneCablesMessageBox)
		{
			rslog::info_ts() << "Patching bytes at " << offset << std::endl;
			Patch_ReplaceWithNops(offset, sizeof(originalBytes_TwoRealToneCablesMessageBox));
		}
	}
}