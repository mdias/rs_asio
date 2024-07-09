#pragma once

#include "RSAsioAudioClientServiceBase.h"

class RSAsioAudioCaptureClient : public RSAsioAudioClientServiceBase<IAudioCaptureClient>
{
public:
	RSAsioAudioCaptureClient(RSAsioAudioClient& asioAudioClient);
	RSAsioAudioCaptureClient(const RSAsioAudioCaptureClient&) = delete;
	RSAsioAudioCaptureClient(RSAsioAudioCaptureClient&&) = delete;

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(BYTE **ppData, UINT32 *pNumFramesToRead, DWORD *pdwFlags, UINT64 *pu64DevicePosition, UINT64 *pu64QPCPosition) override;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesRead) override;
	virtual HRESULT STDMETHODCALLTYPE GetNextPacketSize(UINT32 *pNumFramesInNextPacket) override;

private:
	bool m_WaitingForBufferRelease = false;

	std::mutex m_mutex;
};
