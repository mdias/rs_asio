#include "stdafx.h"
#include "RSAsioAudioClientServiceBase.h"
#include "RSAsioAudioClient.h"
#include "RSAsioDevice.h"

template<typename TBaseClientService>
RSAsioAudioClientServiceBase<TBaseClientService>::RSAsioAudioClientServiceBase(RSAsioAudioClient& asioAudioClient, bool isOutput)
	: m_AsioAudioClient(asioAudioClient)
	, m_NewBufferWaiting(isOutput) // output can immediately fill the backbuffer, input needs to wait for new buffer
{

}

template<typename TBaseClientService>
RSAsioAudioClientServiceBase<TBaseClientService>::~RSAsioAudioClientServiceBase()
{

}

template<typename TBaseClientService>
void RSAsioAudioClientServiceBase<TBaseClientService>::NotifyNewBuffer()
{
	std::lock_guard<std::mutex> g(m_mutex);

	LARGE_INTEGER li;

	if (QueryPerformanceCounter(&li))
		m_NewBufferPerfCounter = li.QuadPart;
	else
		m_NewBufferPerfCounter = 0;

	m_DataDiscontinuityFlag |= m_NewBufferWaiting;
	m_NewBufferWaiting = true;

	if (m_DataDiscontinuityFlag)
	{
		++m_NumSequentialDiscontinuities;
	}
	else
	{
		if (m_NumSequentialDiscontinuities >= 2)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " NotifyNewBuffer() - recovered from " << std::dec << m_NumSequentialDiscontinuities << " discontinuities. Ignoring for some time." << std::endl;
			m_IgnoreDiscontinuityLoggingCountdown = 1000;
		}
		m_NumSequentialDiscontinuities = 0;
	}

	if (m_IgnoreDiscontinuityLoggingCountdown == 0)
	{
		if (m_NumSequentialDiscontinuities == 1)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " NotifyNewBuffer() - data discontinuity" << std::endl;
		}
		else if (m_NumSequentialDiscontinuities == 100)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " NotifyNewBuffer() - data discontinuity x" << std::dec << m_NumSequentialDiscontinuities << ". Not showing any more." << std::endl;
		}
	}
	else
	{
		--m_IgnoreDiscontinuityLoggingCountdown;
	}
}

template<typename TBaseClientService>
bool RSAsioAudioClientServiceBase<TBaseClientService>::HasNewBufferWaiting() const
{
	std::lock_guard<std::mutex> g(m_mutex);

	return m_NewBufferWaiting;
}



template class RSAsioAudioClientServiceBase<IAudioCaptureClient>;
template class RSAsioAudioClientServiceBase<IAudioRenderClient>;
