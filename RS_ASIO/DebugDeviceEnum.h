#pragma once

#include "RSBaseDeviceEnum.h"

class DebugDeviceEnum : public RSBaseDeviceEnum
{
public:
	DebugDeviceEnum(IMMDeviceEnumerator* enumerator);
	DebugDeviceEnum(const DebugDeviceEnum&) = delete;
	DebugDeviceEnum(DebugDeviceEnum&&) = delete;
	virtual ~DebugDeviceEnum();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

	virtual HRESULT STDMETHODCALLTYPE EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection **ppDevices) override;
	virtual HRESULT STDMETHODCALLTYPE GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role, IMMDevice **ppEndpoint) override;
	virtual HRESULT STDMETHODCALLTYPE GetDevice(LPCWSTR pwstrId, IMMDevice **ppDevice) override;
	virtual HRESULT STDMETHODCALLTYPE RegisterEndpointNotificationCallback(IMMNotificationClient *pClient) override;
	virtual HRESULT STDMETHODCALLTYPE UnregisterEndpointNotificationCallback(IMMNotificationClient *pClient) override;

private:
	virtual void UpdateAvailableDevices() override;

	IMMDeviceEnumerator* m_RealEnumerator = nullptr;
};
