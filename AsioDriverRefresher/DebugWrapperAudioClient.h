#pragma once

#include "ComBaseUnknown.h"

template<typename TBase = IAudioClient>
class DebugWrapperAudioClient : public ComBaseUnknown<TBase>
{
public:
	DebugWrapperAudioClient(TBase& realAudioClient, const std::wstring deviceId);
	DebugWrapperAudioClient(const DebugWrapperAudioClient&) = delete;
	DebugWrapperAudioClient(DebugWrapperAudioClient&&) = delete;
	virtual ~DebugWrapperAudioClient();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

	virtual HRESULT STDMETHODCALLTYPE Initialize(AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid) override;
	virtual HRESULT STDMETHODCALLTYPE GetBufferSize(UINT32 *pNumBufferFrames) override;
	virtual HRESULT STDMETHODCALLTYPE GetStreamLatency(REFERENCE_TIME *phnsLatency) override;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentPadding(UINT32 *pNumPaddingFrames) override;
	virtual HRESULT STDMETHODCALLTYPE IsFormatSupported(AUDCLNT_SHAREMODE ShareMode, const WAVEFORMATEX *pFormat, WAVEFORMATEX **ppClosestMatch) override;
	virtual HRESULT STDMETHODCALLTYPE GetMixFormat(WAVEFORMATEX **ppDeviceFormat) override;
	virtual HRESULT STDMETHODCALLTYPE GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod, REFERENCE_TIME *phnsMinimumDevicePeriod) override;
	virtual HRESULT STDMETHODCALLTYPE Start() override;
	virtual HRESULT STDMETHODCALLTYPE Stop() override;
	virtual HRESULT STDMETHODCALLTYPE Reset() override;
	virtual HRESULT STDMETHODCALLTYPE SetEventHandle(HANDLE eventHandle) override;
	virtual HRESULT STDMETHODCALLTYPE GetService(REFIID riid, void **ppv) override;

	const std::wstring GetDeviceId() const;

protected:
	TBase& m_RealAudioClient;
	std::wstring m_DeviceId;

	IAudioCaptureClient* m_CaptureClient = nullptr;
	IAudioRenderClient* m_RenderClient = nullptr;
};

template<typename TBase = IAudioClient2>
class DebugWrapperAudioClient2 : public DebugWrapperAudioClient<TBase>
{
public:
	DebugWrapperAudioClient2(TBase& realAudioClient, const std::wstring deviceId);
	DebugWrapperAudioClient2(const DebugWrapperAudioClient2&) = delete;
	DebugWrapperAudioClient2(DebugWrapperAudioClient2&&) = delete;
	virtual ~DebugWrapperAudioClient2();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

	virtual HRESULT STDMETHODCALLTYPE IsOffloadCapable(AUDIO_STREAM_CATEGORY Category, BOOL *pbOffloadCapable) override;
	virtual HRESULT STDMETHODCALLTYPE SetClientProperties(const AudioClientProperties *pProperties) override;
	virtual HRESULT STDMETHODCALLTYPE GetBufferSizeLimits(const WAVEFORMATEX *pFormat, BOOL bEventDriven, REFERENCE_TIME *phnsMinBufferDuration, REFERENCE_TIME *phnsMaxBufferDuration) override;
};

class DebugWrapperAudioClient3 : public DebugWrapperAudioClient2<IAudioClient3>
{
public:
	DebugWrapperAudioClient3(IAudioClient3& realAudioClient, const std::wstring deviceId);
	DebugWrapperAudioClient3(const DebugWrapperAudioClient3&) = delete;
	DebugWrapperAudioClient3(DebugWrapperAudioClient3&&) = delete;
	virtual ~DebugWrapperAudioClient3();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

	virtual HRESULT STDMETHODCALLTYPE GetSharedModeEnginePeriod(const WAVEFORMATEX *pFormat, UINT32 *pDefaultPeriodInFrames, UINT32 *pFundamentalPeriodInFrames, UINT32 *pMinPeriodInFrames, UINT32 *pMaxPeriodInFrames) override;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSharedModeEnginePeriod(WAVEFORMATEX **ppFormat, UINT32 *pCurrentPeriodInFrames) override;
	virtual HRESULT STDMETHODCALLTYPE InitializeSharedAudioStream(DWORD StreamFlags, UINT32 PeriodInFrames, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid) override;
};