#pragma once

#include "ComBaseUnknown.h"

class DebugWrapperCaptureClient : public ComBaseUnknown<IAudioCaptureClient>
{
public:
	DebugWrapperCaptureClient(IAudioCaptureClient& realClient, const std::wstring& deviceId);
	DebugWrapperCaptureClient(const DebugWrapperCaptureClient&) = delete;
	DebugWrapperCaptureClient(DebugWrapperCaptureClient&&) = delete;
	virtual ~DebugWrapperCaptureClient();

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(BYTE **ppData, UINT32 *pNumFramesToRead, DWORD *pdwFlags, UINT64 *pu64DevicePosition, UINT64 *pu64QPCPosition) override;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesRead) override;
	virtual HRESULT STDMETHODCALLTYPE GetNextPacketSize(UINT32 *pNumFramesInNextPacket) override;

private:
	IAudioCaptureClient& m_RealClient;
	std::wstring m_DeviceId;

	unsigned m_GetCount = 0;
};