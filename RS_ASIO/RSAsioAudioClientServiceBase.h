#pragma once

#include "ComBaseUnknown.h"

class RSAsioAudioClient;

template<typename TBaseClientService>
class RSAsioAudioClientServiceBase : public ComBaseUnknown<TBaseClientService>
{
public:
	RSAsioAudioClientServiceBase(RSAsioAudioClient& asioAudioClient, bool isOutput);
	RSAsioAudioClientServiceBase(const RSAsioAudioClientServiceBase&) = delete;
	RSAsioAudioClientServiceBase(RSAsioAudioClientServiceBase&&) = delete;
	virtual ~RSAsioAudioClientServiceBase();

	void NotifyNewBuffer();
	bool HasNewBufferWaiting() const;

protected:
	RSAsioAudioClient& m_AsioAudioClient;
	bool m_NewBufferWaiting; // set in ctor
	UINT64 m_NewBufferPerfCounter = 0;

	bool m_DataDiscontinuityFlag = false;

	unsigned m_NumSequentialDiscontinuities = 0;
	unsigned m_IgnoreDiscontinuityLoggingCountdown = 0;

	mutable std::mutex m_mutex;
};
