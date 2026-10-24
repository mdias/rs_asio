#include "stdafx.h"
#include "RSAsioAudioEndpointVolume.h"
#include "RSAsioDevice.h"
#include "VolumeTaper.h"

RSAsioAudioEndpointVolume::RSAsioAudioEndpointVolume(RSAsioDevice& asioDevice)
	: m_AsioDevice(asioDevice)
{

}

RSAsioAudioEndpointVolume::~RSAsioAudioEndpointVolume()
{
}

// When [Asio.*] EnableSoftwareEndpointVolumeControl=0 the setters refuse, so the
// rest of the surface must report unsupported too. Advertising a working range
// whose setters then fail would walk the game into its range-aware calibration
// path and strand it there.
bool RSAsioAudioEndpointVolume::IsVolumeControlEnabled() const
{
	return m_AsioDevice.GetConfig().enableSoftwareEndpointVolmeControl;
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
	if (!pnChannelCount)
		return E_POINTER;

	*pnChannelCount = m_AsioDevice.GetNumWasapiChannels();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetMasterVolumeLevel(float fLevelDB, LPCGUID pguidEventContext)
{
	// Negated so NaN is rejected: every ordered comparison against NaN is false,
	// so `fLevelDB < min || fLevelDB > max` would let it through and store a
	// value that reads back as silence but as the topmost step.
	if (!(fLevelDB >= VolumeTaper::kMinDb && fLevelDB <= VolumeTaper::kMaxDb))
		return E_INVALIDARG;

	// GetVolumeRange advertises kStepDb granularity, so the applied setting is the
	// nearest discrete one rather than the exact request. Readback reports what
	// was applied, which keeps the scalar, dB and step views consistent with each
	// other.
	const float appliedDb = VolumeTaper::QuantizeDb(fLevelDB);

	if (!m_AsioDevice.SetEndpointVolumeLevelScalar(VolumeTaper::DbToScalar(appliedDb)))
		return E_FAIL;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::SetMasterVolumeLevelScalar(float fLevel, LPCGUID pguidEventContext)
{
	// Negated so NaN is rejected - see SetMasterVolumeLevel.
	if (!(fLevel >= 0.0f && fLevel <= 1.0f))
		return E_INVALIDARG;

	if (!m_AsioDevice.SetEndpointVolumeLevelScalar(fLevel))
		return E_FAIL;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetMasterVolumeLevel(float *pfLevelDB)
{
	if (!pfLevelDB)
		return E_POINTER;

	if (!IsVolumeControlEnabled())
		return E_NOTIMPL;

	*pfLevelDB = VolumeTaper::ScalarToDb(m_AsioDevice.GetEndpointVolumeLevelScalar());

	return S_OK;
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
	if (!pnStep || !pnStepCount)
		return E_POINTER;

	if (!IsVolumeControlEnabled())
		return E_NOTIMPL;

	*pnStep = VolumeTaper::ScalarToStep(m_AsioDevice.GetEndpointVolumeLevelScalar());
	*pnStepCount = VolumeTaper::kStepCount;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::VolumeStepUp(LPCGUID pguidEventContext)
{
	if (!IsVolumeControlEnabled())
		return E_NOTIMPL;

	const unsigned step = VolumeTaper::ScalarToStep(m_AsioDevice.GetEndpointVolumeLevelScalar());
	if (step + 1 >= VolumeTaper::kStepCount)
		return S_OK;

	if (!m_AsioDevice.SetEndpointVolumeLevelScalar(VolumeTaper::StepToScalar(step + 1)))
		return E_FAIL;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::VolumeStepDown(LPCGUID pguidEventContext)
{
	if (!IsVolumeControlEnabled())
		return E_NOTIMPL;

	const unsigned step = VolumeTaper::ScalarToStep(m_AsioDevice.GetEndpointVolumeLevelScalar());
	if (step == 0)
		return S_OK;

	if (!m_AsioDevice.SetEndpointVolumeLevelScalar(VolumeTaper::StepToScalar(step - 1)))
		return E_FAIL;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::QueryHardwareSupport(DWORD *pdwHardwareSupportMask)
{
	if (!pdwHardwareSupportMask)
		return E_POINTER;

	if (!IsVolumeControlEnabled())
		return E_NOTIMPL;

	// The gain is applied in our own DSP, never by the device. This matches what
	// a real MOTU M Series endpoint reports (measured 0x0); the
	// Realtek onboard endpoint reports 0x3 because it has a hardware preamp.
	*pdwHardwareSupportMask = 0;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioAudioEndpointVolume::GetVolumeRange(float *pflVolumeMindB, float *pflVolumeMaxdB, float *pflVolumeIncrementdB)
{
	if (!pflVolumeMindB || !pflVolumeMaxdB || !pflVolumeIncrementdB)
		return E_POINTER;

	if (!IsVolumeControlEnabled())
		return E_NOTIMPL;

	// The same range a real software endpoint publishes (see VolumeTaper.h), so
	// callers that read the range see one consistent with the applied taper.
	*pflVolumeMindB = VolumeTaper::kMinDb;
	*pflVolumeMaxdB = VolumeTaper::kMaxDb;
	*pflVolumeIncrementdB = VolumeTaper::kStepDb;

	return S_OK;
}
