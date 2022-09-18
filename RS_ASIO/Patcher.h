#pragma once

void PatchOriginalCode();

std::vector<void*> FindBytesOffsets(const BYTE* bytes, size_t numBytes);
void Patch_CallAbsoluteIndirectAddress(const std::vector<void*>& offsets, void* TargetFn);
void Patch_CallRelativeAddress(const std::vector<void*>& offsets, void* TargetFn);
void Patch_ReplaceWithNops(void* offset, size_t numBytes);