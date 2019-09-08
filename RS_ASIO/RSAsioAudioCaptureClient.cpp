#include "stdafx.h"
#include "RSAsioAudioCaptureClient.h"
#include "RSAsioAudioClient.h"
#include "RSAsioDevice.h"

RSAsioAudioCaptureClient::RSAsioAudioCaptureClient(RSAsioAudioClient& asioAudioClient)
	: m_AsioAudioClient(asioAudioClient)
{

}

RSAsioAudioCaptureClient::~RSAsioAudioCaptureClient()
{

}

HRESULT STDMETHODCALLTYPE RSAsioAudioCaptureClient::GetBuffer(BYTE **ppData, UINT32 *pNumFramesToRead, DWORD *pdwFlags, UINT64 *pu64DevicePosition, UINT64 *pu64QPCPosition)
{
	if (!ppData || !pNumFramesToRead || !pdwFlags)
		return E_POINTER;

	if (!m_AsioAudioClient.GetAsioDevice().GetAsioHost().IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	std::vector<BYTE>& buffer = m_AsioAudioClient.GetBuffer();

	*ppData = buffer.data();
	*pNumFramesToRead = m_NewBufferWaiting ? m_AsioAudioClient.GetBufferNumFrames() : 0;

	if (m_NewBufferPerfCounter == 0)
	{
		*pdwFlags = AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR;
	}
	if (!m_NewBufferWaiting)
	{
		*pdwFlags = AUDCLNT_BUFFERFLAGS_SILENT;
	}

	// not implemented
	if (pu64DevicePosition)
	{
		*pu64DevicePosition = 0;
	}

	if (pu64QPCPosition)
	{
		*pu64QPCPosition = m_NewBufferPerfCounter;
	}

	m_WaitingForBufferRelease = true;
	m_NewBufferWaiting = false;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioCaptureClient::ReleaseBuffer(UINT32 NumFramesRead)
{
	if (!m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	m_WaitingForBufferRelease = false;

	//std::cout << ".";

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioCaptureClient::GetNextPacketSize(UINT32 *pNumFramesInNextPacket)
{
	std::cout << __FUNCTION__ "\n";

	if (!pNumFramesInNextPacket)
		return E_POINTER;

	*pNumFramesInNextPacket = m_AsioAudioClient.GetBufferNumFrames();

	return S_OK;
}

void RSAsioAudioCaptureClient::NotifyNewBuffer()
{
	LARGE_INTEGER li;

	if (QueryPerformanceCounter(&li))
		m_NewBufferPerfCounter = li.QuadPart;
	else
		m_NewBufferPerfCounter = 0;

	m_NewBufferWaiting = true;
}