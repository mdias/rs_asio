#include "stdafx.h"
#include "DebugWrapperEndpoint.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) rslog::info_ts() << "  hr: " << HResultToStr(hr) << std::endl

DebugWrapperEndpoint::DebugWrapperEndpoint(IMMEndpoint& realEndpoint, const std::wstring& deviceId)
	: m_RealEndpoint(realEndpoint)
	, m_DeviceId(deviceId)
{
	m_RealEndpoint.AddRef();
}

DebugWrapperEndpoint::~DebugWrapperEndpoint()
{
	m_RealEndpoint.Release();
}

HRESULT STDMETHODCALLTYPE DebugWrapperEndpoint::QueryInterface(REFIID riid, void **ppvObject)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " riid: " << riid << std::endl;

	HRESULT hr = m_RealEndpoint.QueryInterface(riid, ppvObject);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperEndpoint::GetDataFlow(EDataFlow *pDataFlow)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealEndpoint.GetDataFlow(pDataFlow);
	DEBUG_PRINT_HR(hr);

	if (SUCCEEDED(hr))
	{
		rslog::info_ts() << "  *pDataFlow: " << Dataflow2String(*pDataFlow) << std::endl;
	}

	return hr;
}