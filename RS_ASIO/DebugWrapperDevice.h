#pragma once

#include "ComBaseUnknown.h"

class DebugWrapperDevice : public ComBaseUnknown<IMMDevice>
{
public:
	DebugWrapperDevice(IMMDevice& realDevice);
	DebugWrapperDevice(const DebugWrapperDevice&) = delete;
	DebugWrapperDevice(DebugWrapperDevice&&) = delete;
	virtual ~DebugWrapperDevice();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

	virtual HRESULT STDMETHODCALLTYPE Activate(REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface) override;
	virtual HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD stgmAccess, IPropertyStore **ppProperties) override;
	virtual HRESULT STDMETHODCALLTYPE GetId(LPWSTR *ppstrId) override;
	virtual HRESULT STDMETHODCALLTYPE GetState(DWORD *pdwState) override;

private:
	IMMDevice& m_RealDevice;
	std::wstring m_Id;
};