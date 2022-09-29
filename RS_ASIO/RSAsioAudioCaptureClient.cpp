#include "stdafx.h"
#include "RSAsioAudioCaptureClient.h"
#include "RSAsioAudioClient.h"
#include "RSAsioDevice.h"

RSAsioAudioCaptureClient::RSAsioAudioCaptureClient(RSAsioAudioClient& asioAudioClient)
	: RSAsioAudioClientServiceBase<IAudioCaptureClient>(asioAudioClient, /*isOutput =*/false)
{
}

HRESULT STDMETHODCALLTYPE RSAsioAudioCaptureClient::GetBuffer(BYTE **ppData, UINT32 *pNumFramesToRead, DWORD *pdwFlags, UINT64 *pu64DevicePosition, UINT64 *pu64QPCPosition)
{
	std::lock_guard<std::mutex> g(m_mutex);

	if (!ppData || !pNumFramesToRead || !pdwFlags)
		return E_POINTER;

	if (!m_AsioAudioClient.GetAsioDevice().GetAsioHost().IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	// put the newest input data on the backbuffer
	m_AsioAudioClient.SwapBuffers();

	bool isDiscontinuity = true;
	UINT64 backbufferTimestamp = 0;
	BYTE* backbuffer = GetBackbufferIfPending(true, &backbufferTimestamp, &isDiscontinuity);

	*ppData = backbuffer;
	*pNumFramesToRead = backbuffer ? m_AsioAudioClient.GetBufferNumFrames() : 0;

	DWORD dwOutFlags = 0;
	if (backbufferTimestamp == 0)
	{
		dwOutFlags |= AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR;
	}
	if (!backbuffer)
	{
		dwOutFlags |= AUDCLNT_BUFFERFLAGS_SILENT;
	}
	if (isDiscontinuity)
	{
		dwOutFlags |= AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY;
	}
	*pdwFlags = dwOutFlags;

	// not implemented
	if (pu64DevicePosition)
	{
		*pu64DevicePosition = 0;
	}

	if (pu64QPCPosition)
	{
		*pu64QPCPosition = backbufferTimestamp;
	}

	m_WaitingForBufferRelease = true;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioCaptureClient::ReleaseBuffer(UINT32 NumFramesRead)
{
	std::lock_guard<std::mutex> g(m_mutex);

	if (!m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	m_WaitingForBufferRelease = false;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioCaptureClient::GetNextPacketSize(UINT32 *pNumFramesInNextPacket)
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	if (!pNumFramesInNextPacket)
		return E_POINTER;

	*pNumFramesInNextPacket = m_AsioAudioClient.GetBufferNumFrames();

	return S_OK;
}
