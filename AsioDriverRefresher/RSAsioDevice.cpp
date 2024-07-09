#include "stdafx.h"
#include "AsioSharedHost.h"
#include "RSAsioDevice.h"
#include "RSAsioDevicePropertyStore.h"
#include "RSAsioAudioClient.h"
#include "RSAsioAudioEndpointVolume.h"

RSAsioDevice::RSAsioDevice(AsioSharedHost& asioSharedHost, const std::wstring& id, const Config& config)
	: m_AsioSharedHost(asioSharedHost)
	, m_Id(id)
	, m_Config(config)
{
	m_AsioSharedHost.AddRef();
}

RSAsioDevice::~RSAsioDevice()
{
	m_AsioSharedHost.Release();
}

HRESULT STDMETHODCALLTYPE RSAsioDevice::QueryInterface(REFIID riid, void **ppvObject)
{
	if (!ppvObject)
		return E_POINTER;

	if (riid == __uuidof(IMMEndpoint))
	{
		*ppvObject = new RSAsioEndpoint(*this, m_Config.isOutput ? eRender : eCapture);
		return S_OK;
	}

	return ComBaseUnknown<IMMDevice>::QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE RSAsioDevice::Activate(REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface)
{
	if (!ppInterface)
		return E_POINTER;

	if (iid == __uuidof(IAudioClient) || iid == __uuidof(IAudioClient2) || iid == __uuidof(IAudioClient3))
	{
		*ppInterface = new RSAsioAudioClient(*this);
		return S_OK;
	}
	else if (iid == __uuidof(IAudioEndpointVolume))
	{
		*ppInterface = new RSAsioAudioEndpointVolume(*this);
		return S_OK;
	}

	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE RSAsioDevice::OpenPropertyStore(DWORD stgmAccess, IPropertyStore **ppProperties)
{
	if (stgmAccess != STGM_READ)
		return E_FAIL;

	*ppProperties = new RSAsioDevicePropertyStore(*this);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioDevice::GetId(LPWSTR *ppstrId)
{
	if (!ppstrId)
		return E_POINTER;

	const size_t numCharacters = m_Id.length();
	const size_t numBytes = (numCharacters + 1) * sizeof(*m_Id.c_str());
	LPWSTR str = (LPWSTR)CoTaskMemAlloc(numBytes);
	memcpy(str, m_Id.c_str(), numBytes);

	*ppstrId = str;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioDevice::GetState(DWORD *pdwState)
{
	if (!pdwState)
		return E_POINTER;

	*pdwState = DEVICE_STATE_ACTIVE;

	return S_OK;
}

AsioSharedHost& RSAsioDevice::GetAsioHost()
{
	return m_AsioSharedHost;
}

unsigned RSAsioDevice::GetNumWasapiChannels() const
{
	return std::max<unsigned>(m_Config.numAsioChannels, 2);
}

bool RSAsioDevice::SetEndpointVolumeLevelScalar(float fLevel)
{
	if (!m_Config.enableSoftwareEndpointVolmeControl)
		return false;
	m_SoftwareEndpointVolumeScalar = fLevel;
	return true;
}

float RSAsioDevice::GetEndpointVolumeLevelScalar() const
{
	return m_SoftwareEndpointVolumeScalar;
}

bool RSAsioDevice::SetMasterVolumeLevelScalar(float fLevel)
{
	if (!m_Config.enableSoftwareMasterVolumeControl)
		return false;
	m_SoftwareMasterVolumeScalar = fLevel;
	return true;
}

float RSAsioDevice::GetMasterVolumeLevelScalar() const
{
	return m_SoftwareMasterVolumeScalar;
}

bool RSAsioDevice::GetSoftwareVolumeScalar(float* pOutScalar) const
{
	if (!m_Config.enableSoftwareEndpointVolmeControl && !m_Config.enableSoftwareMasterVolumeControl)
		return false;

	if (pOutScalar)
	{
		float volumeScalar = 1.0f;

		if (m_Config.enableSoftwareEndpointVolmeControl)
			volumeScalar *= m_SoftwareEndpointVolumeScalar;

		if (m_Config.enableSoftwareMasterVolumeControl)
			volumeScalar *= m_SoftwareMasterVolumeScalar;

		*pOutScalar = volumeScalar;
	}

	return true;
}



RSAsioEndpoint::RSAsioEndpoint(RSAsioDevice& asioDevice, EDataFlow dataFlow)
	: m_AsioDevice(asioDevice)
	, m_DataFlow(dataFlow)
{
	m_AsioDevice.AddRef();
}

RSAsioEndpoint::~RSAsioEndpoint()
{
	m_AsioDevice.Release();
}

HRESULT RSAsioEndpoint::GetDataFlow(EDataFlow *pDataFlow)
{
	if (!pDataFlow)
		return E_POINTER;

	*pDataFlow = m_DataFlow;

	return S_OK;
}