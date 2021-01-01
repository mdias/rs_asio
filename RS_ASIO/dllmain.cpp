// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Patcher.h"
#include "DebugDeviceEnum.h"
#include "RSAggregatorDeviceEnum.h"
#include "Configurator.h"
#include "MyUnknown.h"

#define USE_STRUCTURED_PA 1

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);

		rslog::InitLog();
		rslog::info_ts() << " - Wrapper DLL loaded (v0.5.5)" << std::endl;
		PatchOriginalCode();
		break;
	case DLL_PROCESS_DETACH:
		rslog::info_ts() << " - Wrapper DLL unloaded" << std::endl;
		rslog::CleanupLog();
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
		break;
    }
    return TRUE;
}

HRESULT STDAPICALLTYPE Patched_CoCreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void **ppOut)
{
	rslog::info_ts() << "Patched_CoCreateInstance called: " << riid << std::endl;

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

#if USE_STRUCTURED_PA
struct PaWasapiSubStream
{
	IUnknown* clientParent;    // IAudioClient
	IUnknown* clientStream;    // IStream
	IUnknown* clientProc;      // IAudioClient
	char dummy[0xdc];
};

struct PaWasapiStream
{
	char dummy[0x108];

	PaWasapiSubStream in;
	IUnknown* captureClientParent; // IAudioCaptureClient
	IUnknown* captureClientStream; // IStream
	IUnknown* captureClient;       // IAudioCaptureClient
	IUnknown* inVol;               // IAudioEndpointVolume

	PaWasapiSubStream out;
	IUnknown* renderClientParent; // IAudioRenderClient
	IUnknown* renderClientStream; // IStream
	IUnknown* renderClient;       // IAudioRenderClient
	IUnknown* outVol;             // IAudioEndpointVolume
};

static void MarshalSubStreamComPointers(PaWasapiSubStream *substream)
{
	substream->clientStream = nullptr;

	if (substream->clientParent)
	{
		substream->clientStream = substream->clientParent;
		substream->clientStream->AddRef();
	}
	else
	{
		if (substream->clientProc)
		{
			substream->clientProc->Release();
			substream->clientProc = nullptr;
		}
	}
}

HRESULT Patched_PortAudio_MarshalStreamComPointers(void* s)
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	PaWasapiStream* stream = reinterpret_cast<PaWasapiStream*>(s);

	if (stream->in.clientParent)
	{
		MarshalSubStreamComPointers(&stream->in);
		stream->captureClientStream = stream->captureClientParent;
		stream->captureClientStream->AddRef();
	}

	if (stream->out.clientParent)
	{
		MarshalSubStreamComPointers(&stream->out);
		stream->renderClientStream = stream->renderClientParent;
		stream->renderClientStream->AddRef();
	}

	return S_OK;
}

static void UnmarshalSubStreamComPointers(PaWasapiSubStream *substream)
{
	if (substream->clientStream)
	{
		substream->clientProc = substream->clientStream;
		substream->clientStream = nullptr;
	}
}

HRESULT Patched_PortAudio_UnmarshalStreamComPointers(void* s)
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	PaWasapiStream* stream = reinterpret_cast<PaWasapiStream*>(s);

	if (stream->in.clientParent)
	{
		UnmarshalSubStreamComPointers(&stream->in);
		stream->captureClient = stream->captureClientStream;
		stream->captureClientStream = nullptr;

		// HACK: this works around the game not calling release on this. could be a bug?
		// this avoids a crash in asio4all, but introduces other possible random crashes
		if (stream->in.clientProc)
		{
			MyUnknown* myUnknown = nullptr;
			stream->in.clientProc->QueryInterface(IID_IMyUnknown, (void**)&myUnknown);
			if (myUnknown)
			{
				if (myUnknown->IsAsio4All)
				{
					rslog::info_ts() << "  using ref count hack" << std::endl;
					stream->in.clientProc->Release();
				}
				myUnknown->Release();
			}
		}
	}

	if (stream->out.clientParent)
	{
		UnmarshalSubStreamComPointers(&stream->out);
		stream->renderClient = stream->renderClientStream;
		stream->renderClientStream = nullptr;
	}

	return S_OK;
}

#else

static const size_t offsetPortAudio_in_ClientParent = 0x108;
static const size_t offsetPortAudio_in_ClientStream = 0x10c;
static const size_t offsetPortAudio_in_ClientProc = 0x110;
static const size_t offsetPortAudio_CaptureClientParent = 0x1f0;
static const size_t offsetPortAudio_CaptureClientStream = 0x1f4;
static const size_t offsetPortAudio_CaptureClient = 0x1f8;

static const size_t offsetPortAudio_RenderClientParent = 0x2e8;
static const size_t offsetPortAudio_RenderClientStream = 0x2ec;
static const size_t offsetPortAudio_RenderClient = 0x2f0;
static const size_t offsetPortAudio_out_ClientParent = 0x200;
static const size_t offsetPortAudio_out_ClientStream = 0x204;
static const size_t offsetPortAudio_out_ClientProc = 0x208;


HRESULT Patched_PortAudio_MarshalStreamComPointers(void* stream)
{
	BYTE* streamBytes = (BYTE*)stream;
	IUnknown*& captureClientParent = *(IUnknown**)&(streamBytes[offsetPortAudio_CaptureClientParent]);
	IUnknown*& captureClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_CaptureClientStream]);
	IUnknown*& in_ClientParent = *(IUnknown**)&(streamBytes[offsetPortAudio_in_ClientParent]);
	IUnknown*& in_ClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_in_ClientStream]);

	IUnknown*& renderClientParent = *(IUnknown**)&(streamBytes[offsetPortAudio_RenderClientParent]);
	IUnknown*& renderClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_RenderClientStream]);
	IUnknown*& out_ClientParent = *(IUnknown**)&(streamBytes[offsetPortAudio_out_ClientParent]);
	IUnknown*& out_ClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_out_ClientStream]);

	rslog::info_ts() << __FUNCTION__ << std::endl;
	captureClientStream = nullptr;
	in_ClientStream = nullptr;
	renderClientStream = nullptr;
	out_ClientStream = nullptr;
	
	if (in_ClientParent != nullptr)
	{
		rslog::info_ts() << "  marshalling input device" << std::endl;

		// (IID_IAudioClient) marshal stream->in->clientParent into stream->in->clientStream
		in_ClientStream = in_ClientParent;
		in_ClientStream->AddRef();

		// (IID_IAudioCaptureClient) marshal stream->captureClientParent onto stream->captureClientStream
		captureClientStream = captureClientParent;
		if (captureClientParent) captureClientParent->AddRef();
	}

	if (out_ClientParent != nullptr)
	{
		rslog::info_ts() << "  marshalling output device" << std::endl;

		// (IID_IAudioClient) marshal stream->out->clientParent into stream->out->clientStream
		out_ClientStream = out_ClientParent;
		out_ClientStream->AddRef();

		// (IID_IAudioRenderClient) marshal stream->renderClientParent onto stream->renderClientStream
		renderClientStream = renderClientParent;
		if (renderClientParent) renderClientParent->AddRef();
	}

	return S_OK;
}

HRESULT Patched_PortAudio_UnmarshalStreamComPointers(void* stream)
{
	BYTE* streamBytes = (BYTE*)stream;
	IUnknown*& captureClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_CaptureClientStream]);
	IUnknown*& captureClient = *(IUnknown**)&(streamBytes[offsetPortAudio_CaptureClient]);
	IUnknown*& in_ClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_in_ClientStream]);
	IUnknown*& in_ClientParent = *(IUnknown**)&(streamBytes[offsetPortAudio_in_ClientParent]);
	IUnknown*& in_ClientProc = *(IUnknown**)&(streamBytes[offsetPortAudio_in_ClientProc]);

	IUnknown*& renderClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_RenderClientStream]);
	IUnknown*& renderClient = *(IUnknown**)&(streamBytes[offsetPortAudio_RenderClient]);
	IUnknown*& out_ClientStream = *(IUnknown**)&(streamBytes[offsetPortAudio_out_ClientStream]);
	IUnknown*& out_ClientParent = *(IUnknown**)&(streamBytes[offsetPortAudio_out_ClientParent]);
	IUnknown*& out_ClientProc = *(IUnknown**)&(streamBytes[offsetPortAudio_out_ClientProc]);

	captureClient = nullptr;
	renderClient = nullptr;
	in_ClientProc = nullptr;
	out_ClientProc = nullptr;

	rslog::info_ts() << __FUNCTION__ << std::endl;

	if (in_ClientParent != nullptr)
	{
		rslog::info_ts() << "  unmarshalling input device" << std::endl;

		// (IID_IAudioClient) unmarshal from stream->in->clientStream into stream->in->clientProc
		in_ClientProc = in_ClientStream;
		in_ClientStream = nullptr;

		// (IID_IAudioCaptureClient) unmarshal from stream->captureClientStream onto stream->captureClient
		captureClient = captureClientStream;
		captureClientStream = nullptr;
	}

	if (out_ClientParent != nullptr)
	{
		rslog::info_ts() << "  unmarshalling output device" << std::endl;

		// (IID_IAudioClient) unmarshal from stream->out->clientStream into stream->out->clientProc
		out_ClientProc = out_ClientStream;
		out_ClientStream = nullptr;

		// (IID_IAudioRenderClient) unmarshal from stream->renderClientStream onto stream->renderClient
		renderClient = renderClientStream;
		renderClientStream = nullptr;
	}

	return S_OK;
}
#endif
