#include "stdafx.h"
#include "DebugWrapperAudioEndpointVolume.h"

#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) std::cout << "  hr: " << HResultToStr(hr) << "\n"

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
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.RegisterControlChangeNotify(pNotify);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::UnregisterControlChangeNotify(IAudioEndpointVolumeCallback *pNotify)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.UnregisterControlChangeNotify(pNotify);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetChannelCount(UINT *pnChannelCount)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetChannelCount(pnChannelCount);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetMasterVolumeLevel(float fLevelDB, LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "fLevelDB: " << fLevelDB << "\n";

	HRESULT hr = m_RealAudioEndpointVolume.SetMasterVolumeLevel(fLevelDB, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetMasterVolumeLevelScalar(float fLevel, LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ " fLevel: " << fLevel << "\n";

	HRESULT hr = m_RealAudioEndpointVolume.SetMasterVolumeLevelScalar(fLevel, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetMasterVolumeLevel(float *pfLevelDB)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetMasterVolumeLevel(pfLevelDB);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetMasterVolumeLevelScalar(float *pfLevel)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetMasterVolumeLevelScalar(pfLevel);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetChannelVolumeLevel(UINT nChannel, float fLevelDB, LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ " nChannel: " << nChannel << " fLevelDB:" << fLevelDB << "\n";

	HRESULT hr = m_RealAudioEndpointVolume.SetChannelVolumeLevel(nChannel, fLevelDB, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetChannelVolumeLevelScalar(UINT nChannel, float fLevel, LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ " nChannel: " << nChannel << " fLevel:" << fLevel << "\n";

	HRESULT hr = m_RealAudioEndpointVolume.SetChannelVolumeLevelScalar(nChannel, fLevel, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetChannelVolumeLevel(UINT nChannel, float *pfLevelDB)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetChannelVolumeLevel(nChannel, pfLevelDB);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetChannelVolumeLevelScalar(UINT nChannel, float *pfLevel)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetChannelVolumeLevelScalar(nChannel, pfLevel);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::SetMute(BOOL bMute, LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ " bMute: " << bMute << "\n";

	HRESULT hr = m_RealAudioEndpointVolume.SetMute(bMute, pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetMute(BOOL *pbMute)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetMute(pbMute);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetVolumeStepInfo(UINT *pnStep, UINT *pnStepCount)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetVolumeStepInfo(pnStep, pnStepCount);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::VolumeStepUp(LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.VolumeStepUp(pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::VolumeStepDown(LPCGUID pguidEventContext)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.VolumeStepDown(pguidEventContext);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::QueryHardwareSupport(DWORD *pdwHardwareSupportMask)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.QueryHardwareSupport(pdwHardwareSupportMask);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioEndpointVolume::GetVolumeRange(float *pflVolumeMindB, float *pflVolumeMaxdB, float *pflVolumeIncrementdB)
{
	std::cout << m_DeviceId << " " __FUNCTION__ "\n";

	HRESULT hr = m_RealAudioEndpointVolume.GetVolumeRange(pflVolumeMindB, pflVolumeMaxdB, pflVolumeIncrementdB);
	DEBUG_PRINT_HR(hr);

	return hr;
}
