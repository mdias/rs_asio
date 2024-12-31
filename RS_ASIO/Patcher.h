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
void Patch_ReplaceWithBytes(void* offset, size_t numBytes, const BYTE* replaceBytes);
