#include "stdafx.h"
#include "RSAsioAudioRenderClient.h"
#include "RSAsioAudioClient.h"
#include "RSAsioDevice.h"

RSAsioAudioRenderClient::RSAsioAudioRenderClient(RSAsioAudioClient& asioAudioClient)
	: m_AsioAudioClient(asioAudioClient)
{
	
}

RSAsioAudioRenderClient::~RSAsioAudioRenderClient()
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

	if (!m_NewBufferWaiting)
	{
		*ppData = nullptr;
		return AUDCLNT_E_BUFFER_ERROR;
	}

	std::vector<BYTE>& buffer = m_AsioAudioClient.GetBackBuffer();
	if (NumFramesRequested != m_AsioAudioClient.GetBufferNumFrames())
		return AUDCLNT_E_BUFFER_SIZE_ERROR;

	*ppData = buffer.data();
	m_WaitingForBufferRelease = true;
	m_NewBufferWaiting = false;

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
	m_DataDiscontinuityFlag = false;

	// move the data we've put in the backbuffer to the frontbuffer for output
	m_AsioAudioClient.SwapBuffers();

	return S_OK;
}

void RSAsioAudioRenderClient::NotifyNewBuffer()
{
	std::lock_guard<std::mutex> g(m_mutex);

	m_DataDiscontinuityFlag |= m_NewBufferWaiting;
	m_NewBufferWaiting = true;

	if (m_DataDiscontinuityFlag)
	{
		++m_NumSequentialDiscontinuities;
	}
	else
	{
		if (m_NumSequentialDiscontinuities >= 2 && m_NumSequentialDiscontinuities==0)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " " __FUNCTION__ " - recovered from " << m_NumSequentialDiscontinuities << " discontinuities. Ignoring for some time." << std::endl;
			m_NumSequentialDiscontinuities = 1000;
		}
		m_NumSequentialDiscontinuities = 0;
	}

	if (m_IgnoreDiscontinuityLoggingCountdown == 0)
	{
		if (m_NumSequentialDiscontinuities == 1)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " " __FUNCTION__ " - data discontinuity" << std::endl;
		}
		else if (m_NumSequentialDiscontinuities == 100)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " " __FUNCTION__ " - data discontinuity x" << m_NumSequentialDiscontinuities << ". Not showing any more." << std::endl;
		}
	}
	else
	{
		--m_NumSequentialDiscontinuities;
	}
}

bool RSAsioAudioRenderClient::HasNewBufferWaiting() const
{
	std::lock_guard<std::mutex> g(m_mutex);

	return m_NewBufferWaiting;
}