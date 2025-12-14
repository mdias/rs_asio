#pragma once

#include <type_traits>
#include <iostream>

#include <unknwn.h>

#include "Log.h"

template <class TBase>
class ComBaseUnknown : public TBase
{
	static_assert(std::is_base_of<IUnknown, TBase>::value, "TBase is not a subclass of IUnknown");
public:
	virtual ~ComBaseUnknown() = default;

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		if (!ppvObject)
			return E_POINTER;

		if (riid == __uuidof(IUnknown))
		{
			*ppvObject = this;
			this->AddRef();
			return S_OK;
		}

		rslog::error_ts() << __FUNCTION__ << " - interface not found; riid: " << riid << std::endl;

		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement(&m_RefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		ULONG ret = InterlockedDecrement(&m_RefCount);
		if (ret == 0)
		{
			delete this;
		}
		return ret;
	}

private:
	LONG m_RefCount = 1;
};
