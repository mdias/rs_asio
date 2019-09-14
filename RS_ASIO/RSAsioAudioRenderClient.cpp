#include "stdafx.h"
#include "RSAsioAudioRenderClient.h"
#include "RSAsioAudioClient.h"
#include "RSAsioDevice.h"

RSAsioAudioRenderClient::RSAsioAudioRenderClient(RSAsioAudioClient& asioAudioClient)
	: m_AsioAudioClient(asioAudioClient)
{
	m_AsioAudioClient.AddRef();
}

RSAsioAudioRenderClient::~RSAsioAudioRenderClient()
{
	m_AsioAudioClient.Release();
}

HRESULT STDMETHODCALLTYPE RSAsioAudioRenderClient::GetBuffer(UINT32 NumFramesRequested, BYTE **ppData)
{
	//rslog::info_ts() << ".";

	if (!ppData)
		return E_POINTER;

	if (!m_AsioAudioClient.GetAsioDevice().GetAsioHost().IsValid())
		return AUDCLNT_E_DEVICE_INVALIDATED;

	if (m_WaitingForBufferRelease)
		return AUDCLNT_E_OUT_OF_ORDER;

	if (m_NewBufferWaiting && NumFramesRequested > 0)
		return AUDCLNT_E_BUFFER_TOO_LARGE;

	std::vector<BYTE>& buffer = m_AsioAudioClient.GetBackBuffer();
	if (NumFramesRequested != m_AsioAudioClient.GetBufferNumFrames())
		return AUDCLNT_E_BUFFER_SIZE_ERROR;

	*ppData = buffer.data();
	m_WaitingForBufferRelease = true;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioRenderClient::ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags)
{
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
	m_NewBufferWaiting = false;

	m_AsioAudioClient.SwapBuffers();

	return S_OK;
}

void RSAsioAudioRenderClient::NotifyNewBuffer()
{
	m_NewBufferWaiting = false;
}