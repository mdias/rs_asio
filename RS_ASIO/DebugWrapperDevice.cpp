#include "stdafx.h"
#include "DebugWrapperDevice.h"
#include "DebugWrapperAudioClient.h"
#include "DebugWrapperDevicePropertyStore.h"
#include "DebugWrapperEndpoint.h"
#include "DebugWrapperAudioEndpointVolume.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) std::cout << "  hr: " << HResultToStr(hr) << std::endl;

DebugWrapperDevice::DebugWrapperDevice(IMMDevice& realDevice)
	: m_RealDevice(realDevice)
{
	realDevice.AddRef();

	LPWSTR pStrId = NULL;
	realDevice.GetId(&pStrId);
	m_Id = pStrId;
	CoTaskMemFree(pStrId);
}

DebugWrapperDevice::~DebugWrapperDevice()
{
	m_RealDevice.Release();
}

HRESULT STDMETHODCALLTYPE DebugWrapperDevice::QueryInterface(REFIID riid, void **ppvObject)
{
	std::cout << m_Id << " " __FUNCTION__" - riid: " << riid << std::endl;

	HRESULT hr = m_RealDevice.QueryInterface(riid, ppvObject);
	DEBUG_PRINT_HR(hr);

	if (SUCCEEDED(hr) && riid == __uuidof(IMMEndpoint) && ppvObject)
	{
		IMMEndpoint* realEndpoint = *(IMMEndpoint**)ppvObject;
		*ppvObject = new DebugWrapperEndpoint(*realEndpoint, m_Id);
		realEndpoint->Release();
		realEndpoint = nullptr;
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperDevice::Activate(REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface)
{
	std::cout << m_Id << " " __FUNCTION__ " - Activate iid: " << iid << " dwClsCtx: " << std::hex << dwClsCtx << std::endl;

	HRESULT hr = E_NOINTERFACE;

	if (!ppInterface)
	{
		hr = E_POINTER;
	}
	else
	{
		if (iid == __uuidof(IAudioClient) || iid == __uuidof(IAudioClient2) || iid == __uuidof(IAudioClient3))
		{
			// first - attempt creating IAudioClient3
			{
				IAudioClient3* realAudioClient = nullptr;
				hr = m_RealDevice.Activate(__uuidof(IAudioClient3), dwClsCtx, pActivationParams, (void**)&realAudioClient);
				if (SUCCEEDED(hr))
				{
					*ppInterface = new DebugWrapperAudioClient3(*realAudioClient, m_Id);
					realAudioClient->Release();
				}
			}

			// if failed, keep trying - with IAudioClient2 if possible
			if (FAILED(hr) && (iid == __uuidof(IAudioClient) || iid == __uuidof(IAudioClient2)))
			{
				IAudioClient2* realAudioClient = nullptr;
				hr = m_RealDevice.Activate(__uuidof(IAudioClient2), dwClsCtx, pActivationParams, (void**)&realAudioClient);
				if (SUCCEEDED(hr))
				{
					*ppInterface = new DebugWrapperAudioClient2<>(*realAudioClient, m_Id);
					realAudioClient->Release();
				}
			}

			// if failed, keep trying - with IAudioClient if possible
			if (FAILED(hr) && iid == __uuidof(IAudioClient))
			{
				IAudioClient* realAudioClient = nullptr;
				hr = m_RealDevice.Activate(__uuidof(IAudioClient), dwClsCtx, pActivationParams, (void**)&realAudioClient);
				if (SUCCEEDED(hr))
				{
					*ppInterface = new DebugWrapperAudioClient<>(*realAudioClient, m_Id);
					realAudioClient->Release();
				}
			}
		}
		else if (iid == __uuidof(IAudioEndpointVolume))
		{
			IAudioEndpointVolume* realInterface = nullptr;
			hr = m_RealDevice.Activate(iid, dwClsCtx, pActivationParams, (void**)&realInterface);
			if (SUCCEEDED(hr) && realInterface)
			{
				*ppInterface = new DebugWrapperAudioEndpointVolume(*realInterface, m_Id);
				realInterface->Release();
			}
		}
		else
		{
			hr = m_RealDevice.Activate(iid, dwClsCtx, pActivationParams, ppInterface);
		}
	}

	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperDevice::OpenPropertyStore(DWORD stgmAccess, IPropertyStore **ppProperties)
{
	std::cout << m_Id << " " __FUNCTION__ " - stgmAccess: " << std::hex << stgmAccess << std::endl;

	IPropertyStore* realPropStore = nullptr;
	HRESULT hr = m_RealDevice.OpenPropertyStore(stgmAccess, &realPropStore);
	DEBUG_PRINT_HR(hr);
	if (SUCCEEDED(hr))
	{
		*ppProperties = new DebugWrapperDevicePropertyStore(*realPropStore, m_Id);
		realPropStore->Release();
		realPropStore = nullptr;
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperDevice::GetId(LPWSTR *ppstrId)
{
	HRESULT hr = m_RealDevice.GetId(ppstrId);
	DEBUG_PRINT_HR(hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperDevice::GetState(DWORD *pdwState)
{
	HRESULT hr = m_RealDevice.GetState(pdwState);
	DEBUG_PRINT_HR(hr);
	return hr;
}