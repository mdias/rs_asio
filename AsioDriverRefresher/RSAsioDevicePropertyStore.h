#pragma once

#include "ComBaseUnknown.h"

class RSAsioDevice;

class RSAsioDevicePropertyStore : public ComBaseUnknown<IPropertyStore>
{
public:
	RSAsioDevicePropertyStore(RSAsioDevice& device);
	virtual ~RSAsioDevicePropertyStore();

	virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *cProps) override;
	virtual HRESULT STDMETHODCALLTYPE GetAt(DWORD iProp, PROPERTYKEY *pkey) override;
	virtual HRESULT STDMETHODCALLTYPE GetValue(REFPROPERTYKEY key, PROPVARIANT *pv) override;
	virtual HRESULT STDMETHODCALLTYPE SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar) override;
	virtual HRESULT STDMETHODCALLTYPE Commit(void) override;

private:
	RSAsioDevice& m_AsioDevice;
};
