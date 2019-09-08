#pragma once

#include "ComBaseUnknown.h"

class DebugWrapperDevicePropertyStore : public ComBaseUnknown<IPropertyStore>
{
public:
	DebugWrapperDevicePropertyStore(IPropertyStore& realPropertyStore, const std::wstring& deviceId);
	DebugWrapperDevicePropertyStore(const DebugWrapperDevicePropertyStore&) = delete;
	DebugWrapperDevicePropertyStore(DebugWrapperDevicePropertyStore&&) = delete;
	virtual ~DebugWrapperDevicePropertyStore();

	virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *cProps) override;
	virtual HRESULT STDMETHODCALLTYPE GetAt(DWORD iProp, PROPERTYKEY *pkey) override;
	virtual HRESULT STDMETHODCALLTYPE GetValue(REFPROPERTYKEY key, PROPVARIANT *pv) override;
	virtual HRESULT STDMETHODCALLTYPE SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar) override;
	virtual HRESULT STDMETHODCALLTYPE Commit(void) override;

private:
	IPropertyStore& m_RealPropertyStore;
	std::wstring m_DeviceId;
};