#include "stdafx.h"
#include "dllmain.h"
#include "crc32.h"

DWORD GetImageCrc32()
{
	char exePath[MAX_PATH]{};
	DWORD exePathSize = GetModuleFileNameA(NULL, exePath, MAX_PATH);

	DWORD crc = 0;
	bool success = crc32file(exePath, crc);
	if (!success)
	{
		rslog::error_ts() << "Could not get the executable crc32" << std::endl;
		return 0;
	}

	return crc;
}

void PatchOriginalCode_d1b38fcb();
void PatchOriginalCode_21a8959a();

std::vector<void*> FindBytesOffsets(const BYTE* bytes, size_t numBytes)
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

void Patch_CallAbsoluteIndirectAddress(const std::vector<void*>& offsets, void* TargetFn)
{
	rslog::info_ts() << __FUNCTION__ " - num locations: " << offsets.size() << std::endl;

	for (void* offset : offsets)
	{
		rslog::info_ts() << "Patching call at " << offset << std::endl;

		long targetRelAddress = (long)TargetFn - ((long)offset + 5);

		BYTE* bytes = (BYTE*)offset;

		DWORD oldProtectFlags = 0;
		if (!VirtualProtect(offset, 6, PAGE_WRITECOPY, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to change memory protection" << std::endl;
		}
		else
		{
			bytes[0] = 0xe8;
			void** callAddress = (void**)(bytes + 1);
			*callAddress = (void*)targetRelAddress;
			bytes[5] = 0x90;

			FlushInstructionCache(GetCurrentProcess(), offset, 6);
			if (!VirtualProtect(offset, 6, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

void Patch_CallRelativeAddress(const std::vector<void*>& offsets, void* TargetFn)
{
	rslog::info_ts() << __FUNCTION__ " - num locations: " << offsets.size() << std::endl;

	for (void* offset : offsets)
	{
		rslog::info_ts() << "Patching call at " << offset << std::endl;

		long targetRelAddress = (long)TargetFn - ((long)offset + 5);

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
			// hack to jump to absolute address without the need to be indirect
			// push address
			bytes[0] = 0x68;
			*((void**)(bytes + 1)) = TargetFn;
			// ret
			bytes[5] = 0xc3;

			if (!VirtualProtect(bytes, 6, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

void Patch_ReplaceWithNops(void* offset, size_t numBytes)
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

void PatchOriginalCode()
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	const DWORD image_crc32 = GetImageCrc32();

	char image_crc32_str[16] = { 0 };
	snprintf(image_crc32_str, 15, "0x%08x", image_crc32);

	rslog::info_ts() << "image crc32: " << image_crc32_str << std::endl;

	switch (image_crc32)
	{
		case 0xd1b38fcb:
			PatchOriginalCode_d1b38fcb();
			break;
		case 0x21a8959a:
			PatchOriginalCode_21a8959a();
			break;
		default:
			rslog::error_ts() << "Unknown game version" << std::endl;
			break;
	}
}