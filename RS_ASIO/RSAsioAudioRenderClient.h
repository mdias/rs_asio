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
	void NotifyUnderrun();
	bool HasNewBufferWaiting() const { return m_NewBufferWaiting; }

private:
	RSAsioAudioClient& m_AsioAudioClient;
	bool m_WaitingForBufferRelease = false;
	bool m_NewBufferWaiting = false;
};
