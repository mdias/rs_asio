#include "stdafx.h"
#include "dllmain.h"
#include "Patcher.h"

// this file contains the patch code for the game version released BEFORE 2022-09-11 (pre-patch)

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

static const BYTE originalBytes_UnknownCrash[]{
	0x8b, 0x8d, 0xc4, 0xfd, 0xff, 0xff,
	0x8b, 0x1c, 0x81
};

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

void PatchOriginalCode_21a8959a()
{
	std::vector<void*> offsets_CoCreateInstance = FindBytesOffsets(originalBytes_call_CoCreateInstance, sizeof(originalBytes_call_CoCreateInstance));
	std::vector<void*> offsets_PaMarshalPointers = FindBytesOffsets(originalBytes_call_PortAudio_MarshalStreamComPointers, sizeof(originalBytes_call_PortAudio_MarshalStreamComPointers));
	std::vector<void*> offsets_PaUnmarshalPointers = FindBytesOffsets(originalBytes_call_UnmarshalStreamComPointers, sizeof(originalBytes_call_UnmarshalStreamComPointers));

	std::vector<void*> offsets_UnknownCrash = FindBytesOffsets(originalBytes_UnknownCrash, sizeof(originalBytes_UnknownCrash));

	if (offsets_CoCreateInstance.size() == 0 && offsets_PaMarshalPointers.size() == 0 && offsets_PaUnmarshalPointers.size() == 0)
	{
		rslog::error_ts() << "No valid locations for patching were found. Make sure you're trying this on the right game version." << std::endl;
	}
	else
	{
		// patch CoCreateInstance calls
		rslog::info_ts() << "Patching CoCreateInstance" << std::endl;
		Patch_CallAbsoluteIndirectAddress(offsets_CoCreateInstance, (void*)&Patched_CoCreateInstance, 1);

		// patch PortAudio MarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio MarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress(offsets_PaMarshalPointers, (void*)&Patched_PortAudio_MarshalStreamComPointers);

		// patch PortAudio UnmarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio UnmarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress(offsets_PaUnmarshalPointers, reinterpret_cast<void*>(&Patched_PortAudio_UnmarshalStreamComPointers));

		Patch_ReplaceAssembly((LPVOID)location_TwoRealToneCablesMessageBox, (LPVOID)patchedBytes_TwoRealToneCablesMessageBox, sizeof(patchedBytes_TwoRealToneCablesMessageBox));

		// patch weird crash (this crash happens even without rs-asio) when certain audio devices are present (like voicemeeter modern versions)
		rslog::info_ts() << "Patching unknown crash when certain audio devices are found (num locations: " << offsets_UnknownCrash.size() << ")" << std::endl;
		for (void* offset : offsets_UnknownCrash)
		{
			const BYTE replaceBytes[]
			{
				0x90, 0x90, 0x90, 0x90, 0x90, // nops...
				0x31, 0xc9, // xor ecx, ecx
				0x31, 0xdb, // xor ebx, ebx
			};
			rslog::info_ts() << "Patching bytes at " << offset << std::endl;
			Patch_ReplaceWithBytes(offset, sizeof(replaceBytes), replaceBytes);
		}
	}
}