#pragma once

#include "RSDeviceCollection.h"
#include "ComBaseUnknown.h"

class RSBaseDeviceEnum : public ComBaseUnknown<IMMDeviceEnumerator>
{
public:
	RSBaseDeviceEnum();
	RSBaseDeviceEnum(const RSBaseDeviceEnum&) = delete;
	RSBaseDeviceEnum(RSBaseDeviceEnum&&) = delete;
	virtual ~RSBaseDeviceEnum();

	virtual HRESULT STDMETHODCALLTYPE EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection **ppDevices) override;
	virtual HRESULT STDMETHODCALLTYPE GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role, IMMDevice **ppEndpoint) override;
	virtual HRESULT STDMETHODCALLTYPE GetDevice(LPCWSTR pwstrId, IMMDevice **ppDevice) override;
	virtual HRESULT STDMETHODCALLTYPE RegisterEndpointNotificationCallback(IMMNotificationClient *pClient) override;
	virtual HRESULT STDMETHODCALLTYPE UnregisterEndpointNotificationCallback(IMMNotificationClient *pClient) override;

	void AddDeviceEnumerator(IMMDeviceEnumerator* enumerator);

protected:
	virtual void UpdateAvailableDevices() = 0;
	virtual void ClearAll();

	bool m_DeviceListNeedsUpdate = false;

	RSDeviceCollection m_RenderDevices;
	RSDeviceCollection m_CaptureDevices;
	std::array<IMMDevice*, ERole_enum_count> m_DefaultRenderDevices;
	std::array<IMMDevice*, ERole_enum_count> m_DefaultCaptureDevices;
};
