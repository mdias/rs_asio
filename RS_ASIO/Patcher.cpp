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

static std::vector<void*> FindBytesOffsets(const BYTE* bytes, size_t numBytes)
{
	std::vector<void*> result;

	const HMODULE baseModuleHandle = GetModuleHandle(NULL);
	MODULEINFO baseModuleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), baseModuleHandle, &baseModuleInfo, sizeof(baseModuleInfo)))
	{
		std::cerr << "Could not get base module info" << std::endl;
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

template<void* NewFn>
static void Patch_CallAbsoluteAddress(const std::vector<void*>& offsets)
{
	std::cout << __FUNCTION__ " - num locations: " << offsets.size() << std::endl;

	static void* fnAddress = NewFn;
	static void* fnAddressIndirect = &fnAddress;

	for (void* offset : offsets)
	{
		std::cout << "Patching call at " << offset << std::endl;

		BYTE* bytes = (BYTE*)offset;

		DWORD oldProtectFlags = 0;
		if (!VirtualProtect(offset, 6, PAGE_WRITECOPY, &oldProtectFlags))
		{
			std::cerr << "Failed to change memory protection" << std::endl;
		}
		else
		{
			void** callAddress = (void**)(bytes + 2);
			*callAddress = fnAddressIndirect;

			if (!VirtualProtect(offset, 6, oldProtectFlags, &oldProtectFlags))
			{
				std::cerr << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

template<void* NewFn>
static void Patch_CallRelativeAddress(const std::vector<void*>& offsets)
{
	std::cout << __FUNCTION__ " - num locations: " << offsets.size() << std::endl;

	static void* fnAddress = NewFn;
	static void* fnAddressIndirect = &fnAddress;

	for (void* offset : offsets)
	{
		std::cout << "Patching call at " << offset << std::endl;

		BYTE* bytes = (BYTE*)offset;

		std::int32_t relOffset = *(std::int32_t*)(bytes + 1);
		bytes += 5 + relOffset;

		DWORD oldProtectFlags = 0;
		if (!VirtualProtect(offset, 6, PAGE_WRITECOPY, &oldProtectFlags))
		{
			std::cerr << "Failed to change memory protection" << std::endl;
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
				std::cerr << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}


void PatchOriginalCode()
{
	std::cout << __FUNCTION__ << std::endl;
	
	// patch CoCreateInstance calls
	{
		std::cout << "Patching CoCreateInstance" << std::endl;

		std::vector<void*> offsets = FindBytesOffsets(originalBytes_call_CoCreateInstance, sizeof(originalBytes_call_CoCreateInstance));
		Patch_CallAbsoluteAddress<(void*)&Patched_CoCreateInstance>(offsets);
	}

	// patch PortAudio MarshalStreamComPointers
	{
		std::cout << "Patching PortAudio MarshalStreamComPointers" << std::endl;
		std::vector<void*> offsets = FindBytesOffsets(originalBytes_call_PortAudio_MarshalStreamComPointers, sizeof(originalBytes_call_PortAudio_MarshalStreamComPointers));
		Patch_CallRelativeAddress<(void*)&Patched_PortAudio_MarshalStreamComPointers>(offsets);
	}

	// patch PortAudio UnmarshalStreamComPointers
	{
		std::cout << "Patching PortAudio UnmarshalStreamComPointers" << std::endl;
		std::vector<void*> offsets = FindBytesOffsets(originalBytes_call_UnmarshalStreamComPointers, sizeof(originalBytes_call_UnmarshalStreamComPointers));
		Patch_CallRelativeAddress<(void*)&Patched_PortAudio_UnmarshalStreamComPointers>(offsets);
	}
}