#pragma once

DWORD crc32buf(const void* buf, size_t len);
bool crc32file(char* name, DWORD& outCrc);