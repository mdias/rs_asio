#pragma once

void InitPatcher();
void DeinitPatcher();
void PatchOriginalCode();
void* GetVirtualProtectFnPtr();
void SetVirtualProtectFnPtr(void* fn);
std::vector<unsigned char> GetUntouchedVirtualProtectBytes(unsigned numBytes);

std::vector<void*> FindBytesOffsets(const BYTE* bytes, size_t numBytes);
void Patch_CallAbsoluteIndirectAddress(const std::vector<void*>& offsets, void* TargetFn, size_t numNopsFollowing=0);
void Patch_CallRelativeAddress(const std::vector<void*>& offsets, void* TargetFn);
void Patch_ReplaceWithNops(void* offset, size_t numBytes);
bool Patch_ReplaceWithBytes(void* offset, size_t numBytes, const BYTE* replaceBytes);
bool Patch_VerifyBytesAtRva(uintptr_t rva, const BYTE* expected, size_t numBytes, const char* description);
bool Patch_VerifyAndReplaceAtRva(uintptr_t rva, const BYTE* expected, const BYTE* replacement, size_t numBytes, const char* description);
void Patch_GameSampleRate(const uintptr_t (&sites)[9], uintptr_t cabinetSiteRva);
bool RedirectedVirtualProtect(LPVOID address, SIZE_T size, DWORD newProtect, PDWORD oldProtect);
