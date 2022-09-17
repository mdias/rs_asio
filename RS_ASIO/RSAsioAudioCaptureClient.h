#pragma once

#include "ComBaseUnknown.h"

class RSAsioAudioClient;

class RSAsioAudioCaptureClient : public ComBaseUnknown<IAudioCaptureClient>
{
public:
	RSAsioAudioCaptureClient(RSAsioAudioClient& asioAudioClient);
	RSAsioAudioCaptureClient(const RSAsioAudioCaptureClient&) = delete;
	RSAsioAudioCaptureClient(RSAsioAudioCaptureClient&&) = delete;
	virtual ~RSAsioAudioCaptureClient();

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(BYTE **ppData, UINT32 *pNumFramesToRead, DWORD *pdwFlags, UINT64 *pu64DevicePosition, UINT64 *pu64QPCPosition) override;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesRead) override;
	virtual HRESULT STDMETHODCALLTYPE GetNextPacketSize(UINT32 *pNumFramesInNextPacket) override;

	void NotifyNewBuffer();
	bool HasNewBufferWaiting() const;

private:
	RSAsioAudioClient& m_AsioAudioClient;
	bool m_NewBufferWaiting = true;
	UINT64 m_NewBufferPerfCounter = 0;

	bool m_WaitingForBufferRelease = false;
	bool m_DataDiscontinuityFlag = false;

	unsigned m_NumSequentialDiscontinuities = 0;
	unsigned m_IgnoreDiscontinuityLoggingCountdown = 0;

	mutable std::mutex m_mutex;
};
