#pragma once

#include "ComBaseUnknown.h"

class RSAsioDevice;

class RSAsioAudioEndpointVolume : public ComBaseUnknown<IAudioEndpointVolume>
{
public:
	RSAsioAudioEndpointVolume(RSAsioDevice& asioDevice);
	RSAsioAudioEndpointVolume(const RSAsioAudioEndpointVolume&) = delete;
	RSAsioAudioEndpointVolume(RSAsioAudioEndpointVolume&&) = delete;
	virtual ~RSAsioAudioEndpointVolume();

	virtual HRESULT STDMETHODCALLTYPE RegisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify) override;
	virtual HRESULT STDMETHODCALLTYPE UnregisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify) override;
	virtual HRESULT STDMETHODCALLTYPE GetChannelCount(UINT *pnChannelCount) override;
	virtual HRESULT STDMETHODCALLTYPE SetMasterVolumeLevel(float fLevelDB, LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE SetMasterVolumeLevelScalar(float fLevel, LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE GetMasterVolumeLevel(float *pfLevelDB) override;
	virtual HRESULT STDMETHODCALLTYPE GetMasterVolumeLevelScalar(float *pfLevel) override;
	virtual HRESULT STDMETHODCALLTYPE SetChannelVolumeLevel(UINT nChannel, float fLevelDB, LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE SetChannelVolumeLevelScalar(UINT nChannel, float fLevel, LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE GetChannelVolumeLevel(UINT nChannel, float *pfLevelDB) override;
	virtual HRESULT STDMETHODCALLTYPE GetChannelVolumeLevelScalar(UINT nChannel, float *pfLevel) override;
	virtual HRESULT STDMETHODCALLTYPE SetMute(BOOL bMute, LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE GetMute(BOOL *pbMute) override;
	virtual HRESULT STDMETHODCALLTYPE GetVolumeStepInfo(UINT *pnStep, UINT *pnStepCount) override;
	virtual HRESULT STDMETHODCALLTYPE VolumeStepUp(LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE VolumeStepDown(LPCGUID pguidEventContext) override;
	virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD *pdwHardwareSupportMask) override;
	virtual HRESULT STDMETHODCALLTYPE GetVolumeRange(float *pflVolumeMindB, float *pflVolumeMaxdB, float *pflVolumeIncrementdB) override;

private:
	RSAsioDevice& m_AsioDevice;
};