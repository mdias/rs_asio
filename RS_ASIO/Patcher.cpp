#include "stdafx.h"
#include "dllmain.h"
#include "crc32.h"

typedef ULONG NTSTATUS;

typedef NTSTATUS(WINAPI* Type_NtProtectVirtualMemory)(HANDLE /*ProcessHandle*/, LPVOID* /*BaseAddress*/, SIZE_T* /*NumberOfBytesToProtect*/, ULONG /*NewAccessProtection*/, PULONG /*OldAccessProtection*/);

static Type_NtProtectVirtualMemory pfnNtProtectVirtualMemory = nullptr;

EXTERN_C ULONG NtProtectVirtualMemory(
	IN HANDLE ProcessHandle,
	IN OUT PVOID* BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG NewProtect,
	OUT PULONG OldProtect
);

static bool RedirectedVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
	if (pfnNtProtectVirtualMemory != nullptr)
	{
		const HANDLE ProcessHandle = GetCurrentProcess();
		SIZE_T NumberOfBytesToProtect = dwSize;
		return (pfnNtProtectVirtualMemory(ProcessHandle, &lpAddress, &NumberOfBytesToProtect, flNewProtect, lpflOldProtect) == 0);
	}

	return NtProtectVirtualMemory(GetCurrentProcess(), &lpAddress, &dwSize, flNewProtect, lpflOldProtect) == 0;
}

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
void PatchOriginalCode_6ea6d1ba();

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

void Patch_CallAbsoluteIndirectAddress(const std::vector<void*>& offsets, void* TargetFn, size_t numNopsFollowing)
{
	rslog::info_ts() << __FUNCTION__ << " - num locations: " << offsets.size() << std::endl;

	for (void* offset : offsets)
	{
		rslog::info_ts() << "Patching call at " << offset << std::endl;

		long targetRelAddress = (long)TargetFn - ((long)offset + 5);

		BYTE* bytes = (BYTE*)offset;

		DWORD oldProtectFlags = 0;
		if (!RedirectedVirtualProtect(offset, 6, PAGE_WRITECOPY, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to change memory protection" << std::endl;
		}
		else
		{
			bytes[0] = 0xe8;
			void** callAddress = (void**)(bytes + 1);
			*callAddress = (void*)targetRelAddress;
			for (size_t i = 0; i < numNopsFollowing; ++i)
			{
				bytes[5+i] = 0x90;
			}

			FlushInstructionCache(GetCurrentProcess(), offset, 5+numNopsFollowing);
			if (!RedirectedVirtualProtect(offset, 5 + numNopsFollowing, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

void Patch_CallRelativeAddress(const std::vector<void*>& offsets, void* TargetFn)
{
	rslog::info_ts() << __FUNCTION__ << " - num locations: " << offsets.size() << std::endl;

	for (void* offset : offsets)
	{
		rslog::info_ts() << "Patching call at " << offset << std::endl;

		long targetRelAddress = (long)TargetFn - ((long)offset + 5);

		BYTE* bytes = (BYTE*)offset;

		std::int32_t relOffset = *(std::int32_t*)(bytes + 1);
		bytes += 5 + relOffset;

		DWORD oldProtectFlags = 0;
		if (!RedirectedVirtualProtect(bytes, 6, PAGE_WRITECOPY, &oldProtectFlags))
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

			if (!RedirectedVirtualProtect(bytes, 6, oldProtectFlags, &oldProtectFlags))
			{
				rslog::error_ts() << "Failed to restore memory protection" << std::endl;
			}
		}
	}
}

void Patch_ReplaceWithNops(void* offset, size_t numBytes)
{
	DWORD oldProtectFlags = 0;
	if (!RedirectedVirtualProtect(offset, numBytes, PAGE_WRITECOPY, &oldProtectFlags))
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
		if (!RedirectedVirtualProtect(offset, numBytes, oldProtectFlags, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to restore memory protection" << std::endl;
		}
	}
}

void Patch_ReplaceWithBytes(void* offset, size_t numBytes, const BYTE* replaceBytes)
{
	DWORD oldProtectFlags = 0;
	if (!RedirectedVirtualProtect(offset, numBytes, PAGE_WRITECOPY, &oldProtectFlags))
	{
		rslog::error_ts() << "Failed to change memory protection" << std::endl;
	}
	else
	{
		BYTE* byte = (BYTE*)offset;
		for (size_t i = 0; i < numBytes; ++i)
		{
			byte[i] = replaceBytes[i];
		}

		FlushInstructionCache(GetCurrentProcess(), offset, numBytes);
		if (!RedirectedVirtualProtect(offset, numBytes, oldProtectFlags, &oldProtectFlags))
		{
			rslog::error_ts() << "Failed to restore memory protection" << std::endl;
		}
	}
}

static bool LoadNtDllFileContents(std::vector<char>& outBuffer)
{
	HMODULE ntDllModule = GetModuleHandleA("ntdll.dll");
	if (!ntDllModule)
	{
		rslog::error_ts() << "Failed to get ntdll.dll module" << std::endl;
		return false;
	}

	char ntDllPath[MAX_PATH + 1]{ 0 };
	if (GetModuleFileNameA(ntDllModule, ntDllPath, sizeof(ntDllPath)) == 0)
	{
		rslog::error_ts() << "Failed to get ntdll.dll path" << std::endl;
		return false;
	}

	FILE* file = fopen(ntDllPath, "rb");
	if (file == nullptr)
	{
		rslog::error_ts() << "Failed to open ntdll.dll for read" << std::endl;
		return false;
	}

	bool result = false;

	if (fseek(file, 0, SEEK_END) != 0)
	{
		rslog::error_ts() << "Failed to get ntdll.dll file size" << std::endl;
	}
	else
	{
		long fileSize = ftell(file);
		if (fileSize > 0)
		{
			outBuffer.resize(fileSize);

			fseek(file, 0, SEEK_SET);

			if (fread(outBuffer.data(), fileSize, 1, file) != 1)
			{
				rslog::error_ts() << "Failed to get ntdll.dll file size" << std::endl;
			}
			else
			{
				result = true;
			}
		}
	}

	fclose(file);
	file = nullptr;

	return result;
}

void InitPatcher()
{
	HMODULE ntdllMod = GetModuleHandleA("ntdll.dll");
	if (!ntdllMod)
	{
		rslog::error_ts() << "Failed get handle for ntdll.dll" << std::endl;
		return;
	}

	pfnNtProtectVirtualMemory = (Type_NtProtectVirtualMemory)GetProcAddress(ntdllMod, "NtProtectVirtualMemory");
	if (!pfnNtProtectVirtualMemory)
	{
		rslog::error_ts() << "Failed get proc address for NtProtectVirtualMemory in ntdll.dll" << std::endl;
		return;
	}
}

void DeinitPatcher()
{
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
		case 0x6ea6d1ba:
			PatchOriginalCode_6ea6d1ba();
			break;
		default:
			rslog::error_ts() << "Unknown game version" << std::endl;
			break;
	}
}

void* GetVirtualProtectFnPtr()
{
	return (void*)pfnNtProtectVirtualMemory;
}

void SetVirtualProtectFnPtr(void* fn)
{
	pfnNtProtectVirtualMemory = (Type_NtProtectVirtualMemory)fn;
}

std::vector<unsigned char> GetUntouchedVirtualProtectBytes(unsigned numBytes)
{
	constexpr const char* fnName = "NtProtectVirtualMemory";

	std::wstring tmpFilePath = GetGamePath() + L"RS_ASIO.ntdll.tmp";
	HMODULE untouchedMod = LoadLibraryW(tmpFilePath.c_str());
	HANDLE tmpFile = nullptr;

	std::vector<char> ntDllFileContents;
	
	if (!untouchedMod)
	{
		// load original
		rslog::info_ts() << "Loading ntdll.dll to memory" << std::endl;
		if (!LoadNtDllFileContents(ntDllFileContents))
		{
			return {};
		}
		else
		{
			rslog::info_ts() << "Loaded ntdll.dll to memory: " << (ntDllFileContents.size() / 1024) << " kB" << std::endl;
		}

		// create temporary copy
		tmpFile = CreateFileW(tmpFilePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_HIDDEN, nullptr);
		if (tmpFile == nullptr)
		{
			rslog::error_ts() << "Failed to open temporary file for writing" << std::endl;
			return {};
		}
		else
		{
			DWORD numBytesWritten = 0;
			const BOOL writeResult = WriteFile(tmpFile, ntDllFileContents.data(), ntDllFileContents.size(), &numBytesWritten, nullptr);
			if (!writeResult || numBytesWritten < ntDllFileContents.size())
			{
				rslog::error_ts() << "Error when writing to temporary file. Wrote " << numBytesWritten << " out of " << ntDllFileContents.size() << " bytes" << std::endl;
			}

			CloseHandle(tmpFile);
			tmpFile = nullptr;
		}

		untouchedMod = LoadLibraryW(tmpFilePath.c_str());
	}

	std::vector<unsigned char> result;

	if (untouchedMod)
	{
		void* proc = (void*)GetProcAddress(untouchedMod, fnName);
		if (!proc)
		{
			rslog::error_ts() << "Failed to get " << fnName << " proc" << std::endl;
		}
		else
		{
			result.resize(numBytes);
			memcpy(result.data(), proc, numBytes);
		}

		FreeLibrary(untouchedMod);
	}
	DeleteFileW(tmpFilePath.c_str());

	return result;
}