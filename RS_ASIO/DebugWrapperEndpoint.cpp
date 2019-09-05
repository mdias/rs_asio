#include "stdafx.h"
#include "DebugWrapperEndpoint.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) std::cout << "  hr: " << HResultToStr(hr) << "\n"

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
	std::cout << m_DeviceId << " " __FUNCTION__ " riid: " << riid << "\n";

	HRESULT hr = m_RealEndpoint.QueryInterface(riid, ppvObject);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperEndpoint::GetDataFlow(EDataFlow *pDataFlow)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealEndpoint.GetDataFlow(pDataFlow);
	DEBUG_PRINT_HR(hr);

	if (SUCCEEDED(hr))
	{
		std::cout << "  *pDataFlow: " << *pDataFlow << "\n";
	}

	return hr;
}