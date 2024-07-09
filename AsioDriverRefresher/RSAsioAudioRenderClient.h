#pragma once

#include "RSAsioAudioClientServiceBase.h"

class RSAsioAudioRenderClient : public RSAsioAudioClientServiceBase<IAudioRenderClient>
{
public:
	RSAsioAudioRenderClient(RSAsioAudioClient& asioAudioClient);
	RSAsioAudioRenderClient(const RSAsioAudioRenderClient&) = delete;
	RSAsioAudioRenderClient(RSAsioAudioRenderClient&&) = delete;

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT32 NumFramesRequested, BYTE **ppData) override;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags) override;

private:
	bool m_WaitingForBufferRelease = false;

	std::mutex m_mutex;
};
