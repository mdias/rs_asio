#pragma once

#include "ComBaseUnknown.h"

class DebugWrapperRenderClient : public ComBaseUnknown<IAudioRenderClient>
{
public:
	DebugWrapperRenderClient(IAudioRenderClient& realClient, const std::wstring& deviceId);
	DebugWrapperRenderClient(const DebugWrapperRenderClient&) = delete;
	DebugWrapperRenderClient(DebugWrapperRenderClient&&) = delete;
	virtual ~DebugWrapperRenderClient();

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT32 NumFramesRequested, BYTE **ppData) override;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags) override;

private:
	IAudioRenderClient& m_RealClient;
	std::wstring m_DeviceId;

	unsigned m_GetCount = 0;
};