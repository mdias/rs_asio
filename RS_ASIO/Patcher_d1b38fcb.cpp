#include "stdafx.h"
#include "dllmain.h"
#include "Patcher.h"
#include "crc32.h"

// this file contains the patch code for the game version released ON 2022-09-11 (post-patch)

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

static const BYTE originalBytes_TwoRealToneCablesMessageBoxStarting[]{
//	0x8b, 0x75, 0x8c, // mov esi, ....
//	0xe8, 0x7c, 0x22, 0xe2, 0xff, // call
//	0x84, 0xc0, // test al, al
	0x74, 0x6b, // jz ...
	0xba, 0x34, 0xfe, 0x1c, 0x01, // mov edx, offset "TooManyGuitarInputs"
	0x8d, 0xb3, 0x4c, 0x01, 0x00, 0x00, // lea esi, [ebx+0x14C]
	0xe8, 0xd8, 0xd8, 0xc4, 0xff // call
};

static const BYTE originalBytes_TwoRealToneCablesMessageBoxMainMenu[]{
	// 0x80, 0xbb, 0x4a, 0x01, 0x00, 0x00, 0x00 // cmp byte ptr [ebx+0x14A], 0
	0x0f, 0x84, 0x95, 0x00, 0x00, 0x00, // jz ...
	0xba, 0x48, 0xfe, 0x1c, 0x01, // mov edx, offset "$[34872]Warning! Two Rocksmith Real Ton..."
	0x8d, 0x75, 0xb0, // lea esi, [ebp+...]
};

static const BYTE originalBytes_UnknownCrash[]{
	0x8b, 0x8d, 0xc4, 0xfd, 0xff, 0xff,
	0x8b, 0x1c, 0x81
};

template<typename T>
void vector_append(std::vector<T>& inOut, const std::vector<T>& source)
{
	for (auto& it : source)
	{
		inOut.push_back(it);
	}
}

void PatchOriginalCode_d1b38fcb()
{
	std::vector<void*> offsets_CoCreateInstanceAbs = FindBytesOffsets(originalBytes_call_CoCreateInstance, sizeof(originalBytes_call_CoCreateInstance));
	vector_append(offsets_CoCreateInstanceAbs, FindBytesOffsets(originalBytes_call_CoCreateInstance2, sizeof(originalBytes_call_CoCreateInstance2)));

	// this is not patching properly, and causing issues with midi stuff in RSMods.
	// we don't seem to need it, so let's keep it disabled.
	//std::vector<void*> offsets_CoCreateInstanceRel = FindBytesOffsets(originalBytes_call_CoCreateInstance3, sizeof(originalBytes_call_CoCreateInstance3));
	//vector_append(offsets_CoCreateInstanceRel, FindBytesOffsets(originalBytes_call_CoCreateInstance4, sizeof(originalBytes_call_CoCreateInstance4)));
	//vector_append(offsets_CoCreateInstanceRel, FindBytesOffsets(originalBytes_call_CoCreateInstance5, sizeof(originalBytes_call_CoCreateInstance5)));

	std::vector<void*> offsets_PaMarshalPointers = FindBytesOffsets(originalBytes_call_PortAudio_MarshalStreamComPointers, sizeof(originalBytes_call_PortAudio_MarshalStreamComPointers));
	std::vector<void*> offsets_PaUnmarshalPointers = FindBytesOffsets(originalBytes_call_UnmarshalStreamComPointers, sizeof(originalBytes_call_UnmarshalStreamComPointers));

	std::vector<void*> offsets_TwoRealToneCablesMessageBoxStarting = FindBytesOffsets(originalBytes_TwoRealToneCablesMessageBoxStarting, sizeof(originalBytes_TwoRealToneCablesMessageBoxStarting));
	std::vector<void*> offsets_TwoRealToneCablesMessageBoxMainMenu = FindBytesOffsets(originalBytes_TwoRealToneCablesMessageBoxMainMenu, sizeof(originalBytes_TwoRealToneCablesMessageBoxMainMenu));

	std::vector<void*> offsets_UnknownCrash = FindBytesOffsets(originalBytes_UnknownCrash, sizeof(originalBytes_UnknownCrash));

	if (offsets_CoCreateInstanceAbs.size() == 0 && offsets_PaMarshalPointers.size() == 0 && offsets_PaUnmarshalPointers.size() == 0)
	{
		rslog::error_ts() << "No valid locations for patching were found. Make sure you're trying this on the right game version." << std::endl;
	}
	else
	{
		// patch CoCreateInstance calls
		rslog::info_ts() << "Patching CoCreateInstance" << std::endl;
		Patch_CallAbsoluteIndirectAddress(offsets_CoCreateInstanceAbs, &Patched_CoCreateInstance, 1);
		//Patch_CallRelativeAddress<(void*)&Patched_CoCreateInstance>(offsets_CoCreateInstanceRel);

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
			const BYTE replaceBytes[]
			{
				0x90, // original instruction at this point is 6 byte wide, we only need 5 bytes, so put a nop here
				0xe9, // jmp rel32
			};
			rslog::info_ts() << "Patching bytes at " << offset << std::endl;
			Patch_ReplaceWithBytes(offset, sizeof(replaceBytes), replaceBytes);
		}

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