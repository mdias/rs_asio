#include "stdafx.h"
#include "RSAsioAudioEndpointVolume.h"
#include "RSAsioDevice.h"

RSAsioAudioEndpointVolume::RSAsioAudioEndpointVolume(RSAsioDevice& asioDevice)
	: m_AsioDevice(asioDevice)
{

}

RSAsioAudioEndpointVolume::~RSAsioAudioEndpointVolume()
{
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::RegisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::UnregisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetChannelCount(UINT *pnChannelCount)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetMasterVolumeLevel(float fLevelDB, LPCGUID pguidEventContext)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetMasterVolumeLevelScalar(float fLevel, LPCGUID pguidEventContext)
{
	if (fLevel < 0.0f || fLevel > 1.0f)
		return E_INVALIDARG;

	if (!m_AsioDevice.SetEndpointVolumeLevelScalar(fLevel))
		return E_FAIL;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetMasterVolumeLevel(float *pfLevelDB)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetMasterVolumeLevelScalar(float *pfLevel)
{
	if (!pfLevel)
		return E_POINTER;

	*pfLevel = m_AsioDevice.GetEndpointVolumeLevelScalar();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetChannelVolumeLevel(UINT nChannel, float fLevelDB, LPCGUID pguidEventContext)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetChannelVolumeLevelScalar(UINT nChannel, float fLevel, LPCGUID pguidEventContext)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetChannelVolumeLevel(UINT nChannel, float *pfLevelDB)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetChannelVolumeLevelScalar(UINT nChannel, float *pfLevel)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetMute(BOOL bMute, LPCGUID pguidEventContext)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetMute(BOOL *pbMute)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetVolumeStepInfo(UINT *pnStep, UINT *pnStepCount)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::VolumeStepUp(LPCGUID pguidEventContext)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::VolumeStepDown(LPCGUID pguidEventContext)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::QueryHardwareSupport(DWORD *pdwHardwareSupportMask)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetVolumeRange(float *pflVolumeMindB, float *pflVolumeMaxdB, float *pflVolumeIncrementdB)
{
	return E_NOTIMPL;
}
