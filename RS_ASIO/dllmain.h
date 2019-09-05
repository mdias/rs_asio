#pragma once

HRESULT STDAPICALLTYPE Patched_CoCreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void **ppOut);
HRESULT Patched_PortAudio_MarshalStreamComPointers(void* stream);
HRESULT Patched_PortAudio_UnmarshalStreamComPointers(void* stream);