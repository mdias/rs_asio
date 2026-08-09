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

	HANDLE file = CreateFileA(ntDllPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (file == INVALID_HANDLE_VALUE)
	{
		rslog::error_ts() << "Failed to open ntdll.dll for read" << std::endl;
		return false;
	}

	bool result = false;

	{
		long fileSize = GetFileSize(file, nullptr);
		if (fileSize > 0)
		{
			outBuffer.resize(fileSize);

			DWORD numBytesRead = 0;
			ReadFile(file, outBuffer.data(), fileSize, &numBytesRead, nullptr);
			if (numBytesRead != fileSize)
			{
				rslog::error_ts() << "Failed to read " << fileSize << " bytes from ntdll.dll" << std::endl;
			}
			else
			{
				result = true;
			}
		}
	}

	CloseHandle(file);

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

	std::vector<char> ntDllFileContents;
	if (!LoadNtDllFileContents(ntDllFileContents))
	{
		return {};
	}
	else
	{
		const DWORD crc32 = crc32buf(ntDllFileContents.data(), ntDllFileContents.size());

		char crc32_str[16] = { 0 };
		snprintf(crc32_str, 15, "0x%08x", crc32);

		rslog::info_ts() << "Loaded ntdll.dll to memory: " << (ntDllFileContents.size() / 1024) << " kB, crc32: " << crc32_str << std::endl;
	}

	// DOS header
	auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(ntDllFileContents.data());
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
	{
		rslog::error_ts() << "  invalid header magic" << std::endl;
		return {};
	}

	// PE32 header
	auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(ntDllFileContents.data() + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		rslog::error_ts() << "  invalid header signature" << std::endl;
		return {};
	}

	// RVA -> file offset
	auto rvaToOffset = [&](DWORD rva) -> size_t
	{
		auto* section = IMAGE_FIRST_SECTION(nt);

		for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
		{
			DWORD start = section->VirtualAddress;
			DWORD size = section->SizeOfRawData;

			if (rva >= start && rva < start + size)
				return section->PointerToRawData + (rva - start);
		}

		return rva;
	};

	const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!dir.VirtualAddress)
	{
		rslog::error_ts() << "  invalid header directory virtual address" << std::endl;
		return {};
	}

	auto* exp = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(ntDllFileContents.data() + rvaToOffset(dir.VirtualAddress));
	auto* names = reinterpret_cast<DWORD*>(ntDllFileContents.data() + rvaToOffset(exp->AddressOfNames));
	auto* ordinals = reinterpret_cast<WORD*>(ntDllFileContents.data() + rvaToOffset(exp->AddressOfNameOrdinals));
	auto* functions = reinterpret_cast<DWORD*>(ntDllFileContents.data() + rvaToOffset(exp->AddressOfFunctions));

	for (DWORD i = 0; i < exp->NumberOfNames; ++i)
	{
		const char* name = reinterpret_cast<const char*>(ntDllFileContents.data() + rvaToOffset(names[i]));

		if (std::strcmp(name, fnName) != 0)
			continue;

		rslog::info_ts() << "  found " << fnName << std::endl;

		DWORD rva = functions[ordinals[i]];
		size_t offset = rvaToOffset(rva);

		if (offset + numBytes > ntDllFileContents.size())
		{
			rslog::error_ts() << "  invalid function offset; not enough bytes present" << std::endl;
			return {};
		}

		std::vector<unsigned char> result;
		result.resize(numBytes);
		memcpy(result.data(), ntDllFileContents.data() + offset, numBytes);

		return result;
	}

	rslog::error_ts() << "  could not find function " << fnName << std::endl;
	return {};
}
