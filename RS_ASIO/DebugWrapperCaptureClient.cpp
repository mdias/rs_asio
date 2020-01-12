#include "stdafx.h"
#include "DebugWrapperCaptureClient.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) rslog::info_ts() << "  hr: " << HResultToStr(hr) << std::endl

DebugWrapperCaptureClient::DebugWrapperCaptureClient(IAudioCaptureClient& realClient, const std::wstring& deviceId)
	: m_RealClient(realClient)
	, m_DeviceId(deviceId)
{
	m_RealClient.AddRef();
}

DebugWrapperCaptureClient::~DebugWrapperCaptureClient()
{
	m_RealClient.Release();
}

HRESULT STDMETHODCALLTYPE DebugWrapperCaptureClient::GetBuffer(BYTE **ppData, UINT32 *pNumFramesToRead, DWORD *pdwFlags, UINT64 *pu64DevicePosition, UINT64 *pu64QPCPosition)
{
	HRESULT hr = E_FAIL;

	if (m_GetCount < 3)
	{
		rslog::info_ts() << m_DeviceId << " " __FUNCTION__  << std::endl;

		hr = m_RealClient.GetBuffer(ppData, pNumFramesToRead, pdwFlags, pu64DevicePosition, pu64QPCPosition);
	}
	else
	{
		hr = m_RealClient.GetBuffer(ppData, pNumFramesToRead, pdwFlags, pu64DevicePosition, pu64QPCPosition);
		if (FAILED(hr))
		{
			rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;
		}
	}

	DEBUG_PRINT_HR(hr);
	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperCaptureClient::ReleaseBuffer(UINT32 NumFramesRead)
{
	HRESULT hr = E_FAIL;

	if (m_GetCount < 3)
	{
		++m_GetCount;
		rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " NumFramesRead: " << NumFramesRead << std::endl;
		hr = m_RealClient.ReleaseBuffer(NumFramesRead);
	}
	else
	{
		hr = m_RealClient.ReleaseBuffer(NumFramesRead);
		if (FAILED(hr))
		{
			rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " NumFramesRead: " << NumFramesRead << std::endl;
		}
	}

	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperCaptureClient::GetNextPacketSize(UINT32 *pNumFramesInNextPacket)
{
	HRESULT hr = E_FAIL;

	if (m_GetCount < 3)
	{
		rslog::info_ts() << m_DeviceId << " " __FUNCTION__  << std::endl;
		hr = m_RealClient.GetNextPacketSize(pNumFramesInNextPacket);
		if (SUCCEEDED(hr))
		{
			rslog::info_ts() << "  *pNumFramesInNextPacket: " << (*pNumFramesInNextPacket) << std::endl;
		}
	}
	else
	{
		hr = m_RealClient.GetNextPacketSize(pNumFramesInNextPacket);
		if (FAILED(hr))
		{
			rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;
		}
	}

	DEBUG_PRINT_HR(hr);

	return hr;
}