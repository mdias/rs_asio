#include "stdafx.h"
#include "DebugWrapperAudioEndpointVolume.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) rslog::info_ts() << "  hr: " << HResultToStr(hr) << std::endl

DebugWrapperAudioEndpointVolume::DebugWrapperAudioEndpointVolume(IAudioEndpointVolume& realAudioEndpointVolume, const std::wstring& deviceId)
	: m_RealAudioEndpointVolume(realAudioEndpointVolume)
	, m_DeviceId(deviceId)
{
	m_RealAudioEndpointVolume.AddRef();
}

DebugWrapperAudioEndpointVolume::~DebugWrapperAudioEndpointVolume()
{
	m_RealAudioEndpointVolume.Release();
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::RegisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.RegisterControlChangeNotify(pNotify);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::UnregisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.UnregisterControlChangeNotify(pNotify);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetChannelCount(UINT *pnChannelCount)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetChannelCount(pnChannelCount);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetMasterVolumeLevel(float fLevelDB, LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ "fLevelDB: " << fLevelDB << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.SetMasterVolumeLevel(fLevelDB, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetMasterVolumeLevelScalar(float fLevel, LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " fLevel: " << fLevel << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.SetMasterVolumeLevelScalar(fLevel, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetMasterVolumeLevel(float *pfLevelDB)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetMasterVolumeLevel(pfLevelDB);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetMasterVolumeLevelScalar(float *pfLevel)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetMasterVolumeLevelScalar(pfLevel);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetChannelVolumeLevel(UINT nChannel, float fLevelDB, LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " nChannel: " << nChannel << " fLevelDB:" << fLevelDB << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.SetChannelVolumeLevel(nChannel, fLevelDB, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetChannelVolumeLevelScalar(UINT nChannel, float fLevel, LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " nChannel: " << nChannel << " fLevel:" << fLevel << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.SetChannelVolumeLevelScalar(nChannel, fLevel, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetChannelVolumeLevel(UINT nChannel, float *pfLevelDB)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetChannelVolumeLevel(nChannel, pfLevelDB);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetChannelVolumeLevelScalar(UINT nChannel, float *pfLevel)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetChannelVolumeLevelScalar(nChannel, pfLevel);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetMute(BOOL bMute, LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ " bMute: " << bMute << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.SetMute(bMute, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetMute(BOOL *pbMute)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetMute(pbMute);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetVolumeStepInfo(UINT *pnStep, UINT *pnStepCount)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetVolumeStepInfo(pnStep, pnStepCount);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::VolumeStepUp(LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.VolumeStepUp(pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::VolumeStepDown(LPCGUID pguidEventContext)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.VolumeStepDown(pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::QueryHardwareSupport(DWORD *pdwHardwareSupportMask)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.QueryHardwareSupport(pdwHardwareSupportMask);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetVolumeRange(float *pflVolumeMindB, float *pflVolumeMaxdB, float *pflVolumeIncrementdB)
{
	rslog::info_ts() << m_DeviceId << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioEndpointVolume.GetVolumeRange(pflVolumeMindB, pflVolumeMaxdB, pflVolumeIncrementdB);
	DEBUG_PRINT_HR(hr);

	return hr;
}
