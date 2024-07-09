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
	std::lock_guard<std::mutex> g(m_bufferMutex);

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
		if (m_NumSequentialDiscontinuities >= 2 && m_IgnoreDiscontinuityLoggingCountdown == 0)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " recovered from " << std::dec << m_NumSequentialDiscontinuities << " consecutive xruns. Ignoring for some time." << std::endl;
			m_IgnoreDiscontinuityLoggingCountdown = 1000;
		}
		m_NumSequentialDiscontinuities = 0;
	}

	if (m_IgnoreDiscontinuityLoggingCountdown == 0)
	{
		if (m_NumSequentialDiscontinuities == 1)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " xrun" << std::endl;
		}
		else if (m_NumSequentialDiscontinuities == 100)
		{
			rslog::info_ts() << m_AsioAudioClient.GetAsioDevice().GetIdRef() << " " << std::dec << m_NumSequentialDiscontinuities << " consecute xruns. Ignoring the next ones." << std::endl;
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
	std::lock_guard<std::mutex> g(m_bufferMutex);

	return m_NewBufferWaiting;
}

template<typename TBaseClientService>
BYTE* RSAsioAudioClientServiceBase<TBaseClientService>::GetBackbufferIfPending(bool resetPendingFlag, UINT64* outOptionalPerfCounter, bool* outOptionalIsDiscontinuity)
{
	std::lock_guard<std::mutex> g(m_bufferMutex);
	if (!m_NewBufferWaiting)
	{
		if (outOptionalPerfCounter)
		{
			*outOptionalPerfCounter = 0;
		}
		if (outOptionalIsDiscontinuity)
		{
			*outOptionalIsDiscontinuity = true;
		}

		return nullptr;
	}

	std::vector<BYTE>& buffer = m_AsioAudioClient.GetBackBuffer();
	if (outOptionalPerfCounter)
	{
		*outOptionalPerfCounter = m_NewBufferPerfCounter;
	}
	if (outOptionalIsDiscontinuity)
	{
		*outOptionalIsDiscontinuity = m_DataDiscontinuityFlag;
	}
	if (resetPendingFlag)
	{
		m_NewBufferWaiting = false;
		m_DataDiscontinuityFlag = false;
	}
	return buffer.data();
}



template class RSAsioAudioClientServiceBase<IAudioCaptureClient>;
template class RSAsioAudioClientServiceBase<IAudioRenderClient>;
