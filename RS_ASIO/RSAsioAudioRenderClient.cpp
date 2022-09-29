#include "stdafx.h"
#include "RSAsioAudioRenderClient.h"
#include "RSAsioAudioClient.h"
#include "RSAsioDevice.h"

RSAsioAudioRenderClient::RSAsioAudioRenderClient(RSAsioAudioClient& asioAudioClient)
	: RSAsioAudioClientServiceBase<IAudioRenderClient>(asioAudioClient, /*isOutput =*/true)
{
}

HRESULT STDMETHODCALLTYPE RSAsioAudioRenderClient::GetBuffer(UINT32 NumFramesRequested, BYTE **ppData)
{
	std::lock_guard<std::mutex> g(m_mutex);

	if (!ppData)
		return E_POINTER;

	if (!m_AsioAudioClient.GetAsioDevice().GetAsioHost().IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	if (NumFramesRequested != m_AsioAudioClient.GetBufferNumFrames())
		return AUDCLNT_E_BUFFER_SIZE_ERROR;

	bool isDiscontinuity = true;
	UINT64 backbufferTimestamp = 0;
	BYTE* backbuffer = GetBackbufferIfPending(true, &backbufferTimestamp, &isDiscontinuity);

	if (!backbuffer)
	{
		*ppData = nullptr;
		return AUDCLNT_E_BUFFER_ERROR;
	}

	*ppData = backbuffer;
	m_WaitingForBufferRelease = true;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioRenderClient::ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags)
{
	std::lock_guard<std::mutex> g(m_mutex);

	if (!m_AsioAudioClient.GetAsioDevice().GetAsioHost().IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (!m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	const DWORD expectedNumFrames = m_AsioAudioClient.GetBufferNumFrames();

	if (NumFramesWritten > expectedNumFrames)
		return AUDCLNT_E_INVALID_SIZE;
	if (NumFramesWritten != expectedNumFrames)
		return AUDCLNT_E_BUFFER_SIZE_ERROR;

	m_WaitingForBufferRelease = false;

	// move the data we've put in the backbuffer to the frontbuffer for output
	m_AsioAudioClient.SwapBuffers();

	return S_OK;
}
