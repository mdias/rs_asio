#pragma once

#include "ComBaseUnknown.h"
#include "AsioSharedHost.h"
#include "MyUnknown.h"

class AsioSharedHost;
class RSAsioDevice;
class RSAsioAudioRenderClient;
class RSAsioAudioCaptureClient;

class RSAsioAudioClient : public ComBaseUnknown<IAudioClient3>, protected IAsioBufferSwitchListener
{
public:
	RSAsioAudioClient(RSAsioDevice& asioDevice);
	RSAsioAudioClient(const RSAsioAudioClient&) = delete;
	RSAsioAudioClient(RSAsioAudioClient&&) = delete;
	virtual ~RSAsioAudioClient();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

	// IAudioClient
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

	// IAudioClient2
	virtual HRESULT STDMETHODCALLTYPE IsOffloadCapable(AUDIO_STREAM_CATEGORY Category, BOOL *pbOffloadCapable) override;
	virtual HRESULT STDMETHODCALLTYPE SetClientProperties(const AudioClientProperties *pProperties) override;
	virtual HRESULT STDMETHODCALLTYPE GetBufferSizeLimits(const WAVEFORMATEX *pFormat, BOOL bEventDriven, REFERENCE_TIME *phnsMinBufferDuration, REFERENCE_TIME *phnsMaxBufferDuration) override;

	// IAudioClient3
	virtual HRESULT STDMETHODCALLTYPE GetSharedModeEnginePeriod(const WAVEFORMATEX *pFormat, UINT32 *pDefaultPeriodInFrames, UINT32 *pFundamentalPeriodInFrames, UINT32 *pMinPeriodInFrames, UINT32 *pMaxPeriodInFrames) override;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSharedModeEnginePeriod(WAVEFORMATEX **ppFormat, UINT32 *pCurrentPeriodInFrames) override;
	virtual HRESULT STDMETHODCALLTYPE InitializeSharedAudioStream(DWORD StreamFlags, UINT32 PeriodInFrames, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid) override;

	RSAsioDevice& GetAsioDevice() { return m_AsioDevice; }

	std::vector<BYTE>& GetBackBuffer() { return m_backBuffer; }
	DWORD GetBufferNumFrames() const { return m_bufferNumFrames; }
	void SwapBuffers();

protected:
	virtual void OnAsioBufferSwitch(unsigned buffIdx) override;

private:
	void UpdateChannelMap();

	RSAsioDevice& m_AsioDevice;
	AsioSharedHost& m_AsioSharedHost;

	WAVEFORMATEXTENSIBLE m_WaveFormat;
	bool m_WaveFormatIsFloat = false;

	bool m_IsInitialized = false;
	bool m_IsStarted = false;
	bool m_UsingEventHandle = false;
	HANDLE m_EventHandle = NULL;

	RSAsioAudioRenderClient* m_RenderClient = nullptr;
	RSAsioAudioCaptureClient* m_CaptureClient = nullptr;

	std::vector<BYTE> m_frontBuffer;
	std::vector<BYTE> m_backBuffer;
	bool m_BuffersWereSwapped = false;
	bool m_IsFirstBuffer = false;

	unsigned m_dbgNumBufferSwitches;

	std::mutex m_bufferMutex;
	std::mutex m_controlMutex;
	DWORD m_bufferNumFrames;

	// for output: for each asio output, which wasapi channel data to copy from
	// for input: for each wasapi channel, which asio channel to read from
	std::vector<int> m_ChannelMap;

	// this is used for ... hacks
	MyUnknown* m_MyUnknown = nullptr;
};