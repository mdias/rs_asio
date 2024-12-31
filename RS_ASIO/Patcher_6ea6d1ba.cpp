#include "stdafx.h"
#include "dllmain.h"
#include "Patcher.h"
#include "crc32.h"

// this file contains the patch code for the game version released ON 2024-12-19 (Rocksmith 2014 Remastered Learn & Play)

static const BYTE originalBytes_call_CoCreateInstance[]{
	0x56, // push esi
	0xe8, 0x0e, 0x16, 0x94, 0x00, // call relative
	0x85, 0xc0, // test eax, eax
	0x0f, 0x88, 0xaa, 0x0c, 0x00, 0x00 // js ...
};

static const BYTE originalBytes_call_PortAudio_MarshalStreamComPointers[]{
	0xe8, 0xec, 0xe4, 0xff, 0xff, // call
	0x83, 0xc4, 0x04, // add esp, 4
	0x85, 0xc0 // test eax, eax
};

static const BYTE originalBytes_call_UnmarshalStreamComPointers[]{
	0xe8, 0xd6, 0xfd, 0xff, 0xff, // call
	0x56, // push esi
	0xe8, 0xb0, 0xfe, 0xff, 0xff // call (to another uninteresting function)
};

static const BYTE originalBytes_TwoRealToneCablesMessageBoxStarting[]{
	0x84, 0xc0, // test al, al
	0x74, 0x6b, // jz ...
	0xba //... mov edx, offset "TooManyGuitarInputs"
	// NOTE: cannot specify the whole "mov" as the offset of the string changes
};

static const BYTE originalBytes_TwoRealToneCablesMessageBoxMainMenu[]{
	0x80, 0xbb, 0x4a, 0x01, 0x00, 0x00, 0x00, // // cmp ...
	0x0f, 0x84, 0x95, 0x00, 0x00, 0x00, // jz ...
	0xba //... mov edx, offset "$[34872]Warning! Two Rocksmith Real Ton..."
	// NOTE: cannot specify the whole "mov" as the offset of the string changes
};

template<typename T>
void vector_append(std::vector<T>& inOut, const std::vector<T>& source)
{
	for (auto& it : source)
	{
		inOut.push_back(it);
	}
}

// the game had modified the first instruction of NtProtectVirtualMemory from "mov" to a
// jump/call that does custom stuff.
// to workaround this we generate a new function with the "mov" from the original data
// and then jump to the instruction after the game-modified mov so that we resume normal
// execution at the correct address.
// the reason we copy the original data instead of just generating the correct "mov" is so
// that both in windows and on linux (with proton) it will still work, as the proton
// NtProtectVirtualMemory version is very similar to the windows one, but the first "mov"
// movs a different value.
static void* GenerateFixedProtectVirtualMemoryFn(void* originalFnPtr)
{
	constexpr unsigned numBytes = 10;

	rslog::info_ts() << "Generating function to patch memory" << std::endl;

	std::vector<unsigned char> origBytes = GetUntouchedVirtualProtectBytes(5);
	if (origBytes.size() == 0)
	{
		rslog::error_ts() << "Failed get original data for patch memory function" << std::endl;
		return nullptr;
	}
	if (origBytes[0] != 0xb8)
	{
		char tmp[8];
		snprintf(tmp, 4, "%02x", origBytes[0]);

		rslog::error_ts() << "Unexpected instruction in original data of patch memory function: " << tmp << std::endl;
		return nullptr;
	}

	const long absDstJumpToOriginal = ((long)(BYTE*)originalFnPtr) + 5; // we want to jump to the instruction following the first "mov"

	BYTE* fnBytes = (BYTE*)VirtualAlloc(NULL, numBytes, MEM_COMMIT, PAGE_READWRITE);
	if (!fnBytes)
	{
		rslog::error_ts() << "Failed to allocate memory for new function" << std::endl;
		return nullptr;
	}
	BYTE* cursorBytes = fnBytes;

	memcpy(cursorBytes, origBytes.data(), 5);
	cursorBytes += 5;

	const long offset = (long)cursorBytes;
	*cursorBytes = 0xe9;
	++cursorBytes;

	const long targetRelAddress = absDstJumpToOriginal - ((long)offset + 5);

	*((long*)cursorBytes) = targetRelAddress;

	DWORD oldProtect = 0;
	if (!VirtualProtect(fnBytes, numBytes, PAGE_EXECUTE, &oldProtect))
	{
		VirtualFree(fnBytes, numBytes, MEM_RELEASE);
		fnBytes = nullptr;
	}

	return fnBytes;
}

void PatchOriginalCode_6ea6d1ba()
{
	void* fnFixedProtectVirtualMemory = GenerateFixedProtectVirtualMemoryFn(GetVirtualProtectFnPtr());
	if (fnFixedProtectVirtualMemory)
	{
		SetVirtualProtectFnPtr(fnFixedProtectVirtualMemory);
	}

	std::vector<void*> offsets_CoCreateInstanceAbs = FindBytesOffsets(originalBytes_call_CoCreateInstance, sizeof(originalBytes_call_CoCreateInstance));

	std::vector<void*> offsets_PaMarshalPointers = FindBytesOffsets(originalBytes_call_PortAudio_MarshalStreamComPointers, sizeof(originalBytes_call_PortAudio_MarshalStreamComPointers));
	std::vector<void*> offsets_PaUnmarshalPointers = FindBytesOffsets(originalBytes_call_UnmarshalStreamComPointers, sizeof(originalBytes_call_UnmarshalStreamComPointers));

	std::vector<void*> offsets_TwoRealToneCablesMessageBoxStarting = FindBytesOffsets(originalBytes_TwoRealToneCablesMessageBoxStarting, sizeof(originalBytes_TwoRealToneCablesMessageBoxStarting));
	std::vector<void*> offsets_TwoRealToneCablesMessageBoxMainMenu = FindBytesOffsets(originalBytes_TwoRealToneCablesMessageBoxMainMenu, sizeof(originalBytes_TwoRealToneCablesMessageBoxMainMenu));

	if (offsets_CoCreateInstanceAbs.size() == 0 && offsets_PaMarshalPointers.size() == 0 && offsets_PaUnmarshalPointers.size() == 0)
	{
		rslog::error_ts() << "No valid locations for patching were found. Make sure you're trying this on the right game version." << std::endl;
	}
	else
	{
		// patch CoCreateInstance calls
		rslog::info_ts() << "Patching CoCreateInstance" << std::endl;
		Patch_CallAbsoluteIndirectAddress(offsets_CoCreateInstanceAbs, &Patched_CoCreateInstance, 1);

		// patch PortAudio MarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio MarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress(offsets_PaMarshalPointers, &Patched_PortAudio_MarshalStreamComPointers);

		// patch PortAudio UnmarshalStreamComPointers
		rslog::info_ts() << "Patching PortAudio UnmarshalStreamComPointers" << std::endl;
		Patch_CallRelativeAddress(offsets_PaUnmarshalPointers, &Patched_PortAudio_UnmarshalStreamComPointers);

		// patch two guitar cables connected message in single-player
		rslog::info_ts() << "Patching Two Guitar Tones Connected Message Box (starting menu) (num locations: " << offsets_TwoRealToneCablesMessageBoxStarting.size() << ")" << std::endl;
		for (void* offset : offsets_TwoRealToneCablesMessageBoxStarting)
		{
			offset = ((BYTE*)offset) + 2;
			const BYTE replaceBytes[]
			{
				0xeb, // jmp rel8
			};
			rslog::info_ts() << "Patching bytes at " << offset << std::endl;
			Patch_ReplaceWithBytes(offset, sizeof(replaceBytes), replaceBytes);
		}

		rslog::info_ts() << "Patching Two Guitar Tones Connected Message Box (main menu) (num locations: " << offsets_TwoRealToneCablesMessageBoxMainMenu.size() << ")" << std::endl;
		for (void* offset : offsets_TwoRealToneCablesMessageBoxMainMenu)
		{
			offset = ((BYTE*)offset) + 7;
			const BYTE replaceBytes[]
			{
				0x90, // original instruction at this point is 6 byte wide, we only need 5 bytes, so put a nop here
				0xe9, // jmp rel32
			};
			rslog::info_ts() << "Patching bytes at " << offset << std::endl;
			Patch_ReplaceWithBytes(offset, sizeof(replaceBytes), replaceBytes);
		}
	}
}