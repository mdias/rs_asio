#include "stdafx.h"
#include "RSBaseDeviceEnum.h"

RSBaseDeviceEnum::RSBaseDeviceEnum()
{
}

RSBaseDeviceEnum::~RSBaseDeviceEnum()
{
	ClearAll();
}

HRESULT STDMETHODCALLTYPE RSBaseDeviceEnum::EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection **ppDevices)
{
	if (m_DeviceListNeedsUpdate)
		UpdateAvailableDevices();

	if (!ppDevices)
		return E_POINTER;

	RSDeviceCollection* newCollection = new RSDeviceCollection();
	if (dataFlow == eRender || dataFlow == eAll)
		newCollection->UpdateDevicesFromCollection(m_RenderDevices, false);
	if (dataFlow == eCapture || dataFlow == eAll)
		newCollection->UpdateDevicesFromCollection(m_CaptureDevices, false);

	newCollection->RemoveIf([dwStateMask](IMMDevice* device) -> bool
		{
			DWORD devState = 0;
			if (FAILED(device->GetState(&devState)))
				return true;
			return (devState & dwStateMask) == 0;
		});
	*ppDevices = newCollection;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSBaseDeviceEnum::GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role, IMMDevice **ppEndpoint)
{
	if (m_DeviceListNeedsUpdate)
		UpdateAvailableDevices();

	if (!ppEndpoint)
		return E_POINTER;

	if (role < 0 || role >= ERole_enum_count)
		return E_INVALIDARG;

	if (dataFlow == eRender)
	{
		if (m_DefaultRenderDevices[role])
		{
			m_DefaultRenderDevices[role]->AddRef();
			*ppEndpoint = m_DefaultRenderDevices[role];
			return S_OK;
		}

		*ppEndpoint = nullptr;
		return E_NOTFOUND;
	}
	else if (dataFlow == eCapture)
	{
		if (m_DefaultCaptureDevices[role])
		{
			m_DefaultCaptureDevices[role]->AddRef();
			*ppEndpoint = m_DefaultCaptureDevices[role];
			return S_OK;
		}

		*ppEndpoint = nullptr;
		return E_NOTFOUND;
	}

	return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE RSBaseDeviceEnum::GetDevice(LPCWSTR pwstrId, IMMDevice **ppDevice)
{
	if (m_DeviceListNeedsUpdate)
		UpdateAvailableDevices();

	if (!ppDevice)
		return E_POINTER;

	IMMDevice* dev = m_RenderDevices.FindById(pwstrId);
	if (!dev)
	{
		dev = m_CaptureDevices.FindById(pwstrId);
	}
	if (dev)
	{
		dev->AddRef();
		*ppDevice = dev;
		return S_OK;
	}

	return E_NOTFOUND;
}

HRESULT STDMETHODCALLTYPE RSBaseDeviceEnum::RegisterEndpointNotificationCallback(IMMNotificationClient *pClient)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSBaseDeviceEnum::UnregisterEndpointNotificationCallback(IMMNotificationClient *pClient)
{
	return E_NOTIMPL;
}

void RSBaseDeviceEnum::ClearAll()
{
	m_RenderDevices.Clear();
	m_CaptureDevices.Clear();

	for (IMMDevice*& dev : m_DefaultRenderDevices)
	{
		if (dev)
		{
			dev->Release();
			dev = nullptr;
		}
	}
	for (IMMDevice*& dev : m_DefaultCaptureDevices)
	{
		if (dev)
		{
			dev->Release();
			dev = nullptr;
		}
	}
}