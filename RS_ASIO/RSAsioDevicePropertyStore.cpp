#include "stdafx.h"
#include "RSAsioDevicePropertyStore.h"
#include "RSAsioDevice.h"
#include "AsioSharedHost.h"

#include <functiondiscoverykeys_devpkey.h>

RSAsioDevicePropertyStore::RSAsioDevicePropertyStore(RSAsioDevice& device)
	: m_AsioDevice(device)
{
	m_AsioDevice.AddRef();
}

RSAsioDevicePropertyStore::~RSAsioDevicePropertyStore()
{
	m_AsioDevice.Release();
}

HRESULT STDMETHODCALLTYPE RSAsioDevicePropertyStore::GetCount(DWORD *cProps)
{
	if (!cProps)
		return E_POINTER;

	if (m_AsioDevice.GetConfig().isOutput)
		*cProps = 2;
	else
	{
		if (m_AsioDevice.GetConfig().isMicrophone)
			*cProps = 3;
		else
			*cProps = 5;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioDevicePropertyStore::GetAt(DWORD iProp, PROPERTYKEY *pkey)
{
	switch(iProp)
	{
	case 0:
		*pkey = PKEY_AudioEngine_DeviceFormat;
		return S_OK;
	case 1:
		*pkey = PKEY_Device_FriendlyName;
		return S_OK;
	case 2:
		*pkey = PKEY_AudioEndpoint_FormFactor;
		return S_OK;
	case 3:
		*pkey = PKEY_Device_DeviceIdHiddenKey1;
		return S_OK;
	case 4:
		*pkey = PKEY_Device_DeviceIdHiddenKey2;
		return S_OK;
	}

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE RSAsioDevicePropertyStore::GetValue(REFPROPERTYKEY key, PROPVARIANT *pv)
{
	//rslog::info_ts() << __FUNCTION__ " - key: " << key << std::endl;

	if (!pv)
		return E_POINTER;

	pv->vt = VT_EMPTY;

	AsioSharedHost& asioHost = m_AsioDevice.GetAsioHost();
	if (asioHost.IsValid())
	{
		if (key == PKEY_AudioEngine_DeviceFormat)
		{
			IAsioDriver* asioDriver = asioHost.GetDriver();

			long numInputChannels = 0;
			long numOutputChannels = 0;
			ASIOSampleRate sampleRate = 0.0;

			if (asioDriver->getChannels(&numInputChannels, &numOutputChannels) == ASE_OK
				&& asioDriver->getSampleRate(&sampleRate) == ASE_OK)
			{
				WAVEFORMATEXTENSIBLE* wfe = (WAVEFORMATEXTENSIBLE*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
				wfe->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
				wfe->Format.nChannels = m_AsioDevice.GetNumWasapiChannels();
				wfe->Format.wBitsPerSample = 32;
				wfe->Format.nSamplesPerSec = std::lround(sampleRate);
				wfe->Format.nBlockAlign = wfe->Format.nChannels * (wfe->Format.wBitsPerSample / 8);
				wfe->Format.nAvgBytesPerSec = wfe->Format.nBlockAlign * wfe->Format.nSamplesPerSec;
				wfe->Format.cbSize = 22;
				wfe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
				wfe->Samples.wSamplesPerBlock = 24;
				wfe->dwChannelMask = 0;

				for (WORD i = 0; i < wfe->Format.nChannels; ++i)
				{
					wfe->dwChannelMask = (wfe->dwChannelMask << 1) | 1;
				}
				
				pv->vt = VT_BLOB;
				pv->blob.pBlobData = (BYTE*)wfe;
				pv->blob.cbSize = sizeof(*wfe);
			}
		}
		else if (key == PKEY_Device_FriendlyName)
		{
			if (SUCCEEDED(m_AsioDevice.GetId(&pv->pwszVal)))
			{
				pv->vt = VT_LPWSTR;
			}
		}
		else if (key == PKEY_Device_DeviceIdHiddenKey1)
		{
			const wchar_t strDeviceId[] = L"{1}.FAKEDEVICE\\VID_12BA&PID_00FF&KS\\YOLO";

			if (pv->pwszVal = (LPWSTR)CoTaskMemAlloc(sizeof(strDeviceId)))
			{
				memcpy(pv->pwszVal, strDeviceId, sizeof(strDeviceId));
				pv->vt = VT_LPWSTR;
			}
		}
		else if (key == PKEY_Device_DeviceIdHiddenKey2)
		{
			const wchar_t strDeviceId[] = L"oem24.inf:0000000000000000:_Install_8.NTamd64:4.59.0.56775:FAKEDEVICE\\VID_12BA&PID_00FF&KS";

			if (pv->pwszVal = (LPWSTR)CoTaskMemAlloc(sizeof(strDeviceId)))
			{
				memcpy(pv->pwszVal, strDeviceId, sizeof(strDeviceId));
				pv->vt = VT_LPWSTR;
			}
		}
		else if (key == PKEY_AudioEndpoint_FormFactor)
		{
			pv->vt = VT_UI4;
			pv->uintVal = m_AsioDevice.GetConfig().isMicrophone ? Microphone : LineLevel;
		}
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSAsioDevicePropertyStore::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE RSAsioDevicePropertyStore::Commit(void)
{
	rslog::info_ts() << __FUNCTION__ << std::endl;

	return STG_E_ACCESSDENIED;
}
