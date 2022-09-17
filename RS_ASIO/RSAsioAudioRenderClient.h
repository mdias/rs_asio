#pragma once

#include "ComBaseUnknown.h"

class RSAsioAudioClient;

class RSAsioAudioRenderClient : public ComBaseUnknown<IAudioRenderClient>
{
public:
	RSAsioAudioRenderClient(RSAsioAudioClient& asioAudioClient);
	RSAsioAudioRenderClient(const RSAsioAudioRenderClient&) = delete;
	RSAsioAudioRenderClient(RSAsioAudioRenderClient&&) = delete;
	virtual ~RSAsioAudioRenderClient();

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT32 NumFramesRequested, BYTE **ppData) override;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags) override;

	void NotifyNewBuffer();
	bool HasNewBufferWaiting() const;

private:
	RSAsioAudioClient& m_AsioAudioClient;
	bool m_WaitingForBufferRelease = false;
	bool m_NewBufferWaiting = true;
	bool m_DataDiscontinuityFlag = false;

	unsigned m_NumSequentialDiscontinuities = 0;
	unsigned m_IgnoreDiscontinuityLoggingCountdown = 0;

	mutable std::mutex m_mutex;
};
