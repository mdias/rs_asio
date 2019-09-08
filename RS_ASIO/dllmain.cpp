// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Patcher.h"
#include "DebugDeviceEnum.h"
#include "RSAggregatorDeviceEnum.h"
#include "Configurator.h"

static std::ofstream fStreamLog;

static void InitConsole()
{
	fStreamLog.open("RS_ASIO-log.txt", std::ios_base::out | std::ios_base::trunc);

	std::cout.set_rdbuf(fStreamLog.rdbuf());
	std::cerr.set_rdbuf(fStreamLog.rdbuf());
}

static void CleanupConsole()
{
	fStreamLog.close();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		InitConsole();
		DisableThreadLibraryCalls(hModule);
		std::cout << "Wrapper DLL loaded\n";
		PatchOriginalCode();
		break;
	case DLL_PROCESS_DETACH:
		std::cout << "Wrapper DLL unloaded\n";
		CleanupConsole();
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
    }
    return TRUE;
}

HRESULT STDAPICALLTYPE Patched_CoCreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void **ppOut)
{
	std::cout << "Patched_CoCreateInstance called: " << riid << "\n";

	if (!ppOut)
		return E_POINTER;

	if (riid == __uuidof(IMMDeviceEnumerator))
	{
		RSAggregatorDeviceEnum* aggregatorEnum = new RSAggregatorDeviceEnum();
		SetupDeviceEnumerator(*aggregatorEnum);

		DebugDeviceEnum* newEnum = new DebugDeviceEnum(aggregatorEnum);
		aggregatorEnum->Release();

		*ppOut = newEnum;
		return S_OK;
	}

	return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppOut);
}

static const size_t offsetPortAudio_CaptureClientParent = 0x1f0;
static const size_t offsetPortAudio_CaptureClientStream = 0x1f4;
static const size_t offsetPortAudio_CaptureClient = 0x1f8;
static const size_t offsetPortAudio_in_ClientStream = 0x10c;
static const size_t offsetPortAudio_in_ClientParent = 0x108;
static const size_t offsetPortAudio_in_ClientProc = 0x110;

static const size_t offsetPortAudio_RenderClientParent = 0x2e8;
static const size_t offsetPortAudio_RenderClientStream = 0x2ec;
static const size_t offsetPortAudio_RenderClient = 0x2f0;
static const size_t offsetPortAudio_out_ClientStream = 0x204;
static const size_t offsetPortAudio_out_ClientParent = 0x200;
static const size_t offsetPortAudio_out_ClientProc = 0x208;


HRESULT Patched_PortAudio_MarshalStreamComPointers(void* stream)
{
	BYTE* streamBytes = (BYTE*)stream;
	void** captureClientParent = (void**)&(streamBytes[offsetPortAudio_CaptureClientParent]);
	void** captureClientStream = (void**)&(streamBytes[offsetPortAudio_CaptureClientStream]);
	void** in_ClientParent = (void**)&(streamBytes[offsetPortAudio_in_ClientParent]);
	void** in_ClientStream = (void**)&(streamBytes[offsetPortAudio_in_ClientStream]);

	void** renderClientParent = (void**)&(streamBytes[offsetPortAudio_RenderClientParent]);
	void** renderClientStream = (void**)&(streamBytes[offsetPortAudio_RenderClientStream]);
	void** out_ClientParent = (void**)&(streamBytes[offsetPortAudio_out_ClientParent]);
	void** out_ClientStream = (void**)&(streamBytes[offsetPortAudio_out_ClientStream]);

	std::cout << __FUNCTION__ "\n";
	*captureClientStream = nullptr;
	*in_ClientStream = nullptr;
	*renderClientStream = nullptr;
	*out_ClientStream = nullptr;
	
	if ((*in_ClientParent) != nullptr)
	{
		std::cout << "  marshalling input device\n";

		// (IID_IAudioClient) marshal stream->in->clientParent into stream->in->clientStream
		*in_ClientStream = *in_ClientParent;

		// (IID_IAudioCaptureClient) marshal stream->captureClientParent onto stream->captureClientStream
		*captureClientStream = *captureClientParent;
	}

	if ((*out_ClientParent) != nullptr)
	{
		std::cout << "  marshalling output device\n";

		// (IID_IAudioClient) marshal stream->out->clientParent into stream->out->clientStream
		*out_ClientStream = *out_ClientParent;

		// (IID_IAudioRenderClient) marshal stream->renderClientParent onto stream->renderClientStream
		*renderClientStream = *renderClientParent;
	}

	return S_OK;
}

HRESULT Patched_PortAudio_UnmarshalStreamComPointers(void* stream)
{
	BYTE* streamBytes = (BYTE*)stream;
	void** captureClientStream = (void**)&(streamBytes[offsetPortAudio_CaptureClientStream]);
	void** captureClient = (void**)&(streamBytes[offsetPortAudio_CaptureClient]);
	void** in_ClientStream = (void**)&(streamBytes[offsetPortAudio_in_ClientStream]);
	void** in_ClientParent = (void**)&(streamBytes[offsetPortAudio_in_ClientParent]);
	void** in_ClientProc = (void**)&(streamBytes[offsetPortAudio_in_ClientProc]);

	void** renderClientStream = (void**)&(streamBytes[offsetPortAudio_RenderClientStream]);
	void** renderClient = (void**)&(streamBytes[offsetPortAudio_RenderClient]);
	void** out_ClientStream = (void**)&(streamBytes[offsetPortAudio_out_ClientParent]);
	void** out_ClientParent = (void**)&(streamBytes[offsetPortAudio_out_ClientParent]);
	void** out_ClientProc = (void**)&(streamBytes[offsetPortAudio_out_ClientProc]);

	*captureClient = nullptr;
	*renderClient = nullptr;
	*in_ClientProc = nullptr;
	*out_ClientProc = nullptr;

	std::cout << __FUNCTION__ "\n";

	if ((*in_ClientParent) != nullptr)
	{
		std::cout << "  unmarshalling input device\n";

		// (IID_IAudioClient) unmarshal from stream->in->clientStream into stream->in->clientProc
		*in_ClientProc = *in_ClientStream;
		*in_ClientStream = nullptr;

		// (IID_IAudioCaptureClient) unmarshal from stream->captureClientStream onto stream->captureClient
		*captureClient = *captureClientStream;
		*captureClientStream = nullptr;
	}

	if ((*out_ClientParent) != nullptr)
	{
		std::cout << "  unmarshalling output device\n";

		// (IID_IAudioClient) unmarshal from stream->out->clientStream into stream->out->clientProc
		*out_ClientProc = *out_ClientStream;
		*out_ClientStream = nullptr;

		// (IID_IAudioRenderClient) unmarshal from stream->renderClientStream onto stream->renderClient
		*renderClient = *renderClientStream;
		*renderClientStream = nullptr;
	}

	return S_OK;
}
