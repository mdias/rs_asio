#include "stdafx.h"
#include "DebugWrapperRenderClient.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) rslog::info_ts() << "  hr: " << HResultToStr(hr) << std::endl

DebugWrapperRenderClient::DebugWrapperRenderClient(IAudioRenderClient& realClient, const std::wstring& deviceId)
	: m_RealClient(realClient)
	, m_DeviceId(deviceId)
{
	m_RealClient.AddRef();
}

DebugWrapperRenderClient::~DebugWrapperRenderClient()
{
	m_RealClient.Release();
}

HRESULT STDMETHODCALLTYPE DebugWrapperRenderClient::GetBuffer(UINT32 NumFramesRequested, BYTE **ppData)
{
	HRESULT hr = E_FAIL;

	if (m_GetCount < 3)
	{
		rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " NumFramesRequested: " << NumFramesRequested << std::endl;

		hr = m_RealClient.GetBuffer(NumFramesRequested, ppData);
	}
	else
	{
		hr = m_RealClient.GetBuffer(NumFramesRequested, ppData);
		if (FAILED(hr))
		{
			rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " NumFramesRequested: " << NumFramesRequested << std::endl;
		}
	}

	DEBUG_PRINT_HR(hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperRenderClient::ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags)
{
	HRESULT hr = E_FAIL;

	if (m_GetCount < 3)
	{
		++m_GetCount;
		rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " NumFramesWritten: " << NumFramesWritten << std::endl;
		hr = m_RealClient.ReleaseBuffer(NumFramesWritten, dwFlags);
	}
	else
	{
		hr = m_RealClient.ReleaseBuffer(NumFramesWritten, dwFlags);
		if (FAILED(hr))
		{
			rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " NumFramesWritten: " << NumFramesWritten << std::endl;
		}
	}

	DEBUG_PRINT_HR(hr);

	return hr;
}