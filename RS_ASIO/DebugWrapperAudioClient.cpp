#include "stdafx.h"
#include "DebugWrapperAudioClient.h"


#define DEBUG_PRINT_HR(hr) if(FAILED(hr)) rslog::info_ts() << "  hr: " << HResultToStr(hr) << std::endl

template<typename TBase>
DebugWrapperAudioClient<TBase>::DebugWrapperAudioClient(TBase& realAudioClient, const std::wstring deviceId)
	: m_RealAudioClient(realAudioClient)
	, m_DeviceId(deviceId)
{
	m_RealAudioClient.AddRef();
}

template<typename TBase>
DebugWrapperAudioClient<TBase>::~DebugWrapperAudioClient()
{
	m_RealAudioClient.Release();
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::QueryInterface(REFIID riid, void **ppvObject)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << " riid: " << riid << std::endl;

	HRESULT hr = E_POINTER;

	if (ppvObject)
	{
		if (riid == __uuidof(IAudioClient))
		{
			*ppvObject = this;
			this->AddRef();
			hr = S_OK;
		}
		else
		{
			hr = ComBaseUnknown<TBase>::QueryInterface(riid, ppvObject);
		}
	}
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::Initialize(AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ " - ShareMode: " << ShareMode << " Flags: " << std::hex << StreamFlags << " bufferDuration: " << std::dec << RefTimeToMilisecs(hnsBufferDuration) << "ms periodicity: " << RefTimeToMilisecs(hnsPeriodicity) << "ms" << std::endl;

	HRESULT hr = m_RealAudioClient.Initialize(ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat, AudioSessionGuid);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::GetBufferSize(UINT32 *pNumBufferFrames)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;
	
	HRESULT hr = m_RealAudioClient.GetBufferSize(pNumBufferFrames);
	DEBUG_PRINT_HR(hr);

	if (SUCCEEDED(hr))
	{
		rslog::info_ts() << "  *pNumBufferFrames: " << std::dec << *pNumBufferFrames << std::endl;
	}

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::GetStreamLatency(REFERENCE_TIME *phnsLatency)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.GetStreamLatency(phnsLatency);
	DEBUG_PRINT_HR(hr);

	if (SUCCEEDED(hr))
	{
		rslog::info_ts() << "  latency: " << std::dec << RefTimeToMilisecs(*phnsLatency) << "ms" << std::endl;
	}

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::GetCurrentPadding(UINT32 *pNumPaddingFrames)
{
	//rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.GetCurrentPadding(pNumPaddingFrames);
	//rslog::info_ts() << "  hr: " << HResultToStr(hr) << std::endl;

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::IsFormatSupported(AUDCLNT_SHAREMODE ShareMode, const WAVEFORMATEX *pFormat, WAVEFORMATEX **ppClosestMatch)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ " - ShareMode: " << ShareMode << std::endl;

	if (!pFormat)
		return E_POINTER;

	HRESULT hr = m_RealAudioClient.IsFormatSupported(ShareMode, pFormat, ppClosestMatch);
	DEBUG_PRINT_HR(hr);

	if (FAILED(hr))
	{
		rslog::info_ts() << (*pFormat);
	}

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::GetMixFormat(WAVEFORMATEX **ppDeviceFormat)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;
	
	HRESULT hr = m_RealAudioClient.GetMixFormat(ppDeviceFormat);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod, REFERENCE_TIME *phnsMinimumDevicePeriod)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.GetDevicePeriod(phnsDefaultDevicePeriod, phnsMinimumDevicePeriod);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::Start()
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.Start();
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::Stop()
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.Stop();
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::Reset()
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.Reset();
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::SetEventHandle(HANDLE eventHandle)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.SetEventHandle(eventHandle);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient<TBase>::GetService(REFIID riid, void **ppv)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << " - riid: " << riid << std::endl;

	HRESULT hr = m_RealAudioClient.GetService(riid, ppv);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
const std::wstring DebugWrapperAudioClient<TBase>::GetDeviceId() const
{
	return m_DeviceId;
}


template class DebugWrapperAudioClient<IAudioClient>;
template class DebugWrapperAudioClient<IAudioClient2>;
template class DebugWrapperAudioClient<IAudioClient3>;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename TBase>
DebugWrapperAudioClient2<TBase>::DebugWrapperAudioClient2(TBase& realAudioClient, const std::wstring deviceId)
	: DebugWrapperAudioClient<TBase>(realAudioClient, deviceId)
{
}

template<typename TBase>
DebugWrapperAudioClient2<TBase>::~DebugWrapperAudioClient2()
{
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient2<TBase>::QueryInterface(REFIID riid, void **ppvObject)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << " riid: " << riid << std::endl;

	if (!ppvObject)
	{
		HRESULT hr = E_POINTER;
		DEBUG_PRINT_HR(hr);

		return hr;
	}

	if (riid == __uuidof(IAudioClient2))
	{
		*ppvObject = this;
		this->AddRef();
		return S_OK;
	}

	return DebugWrapperAudioClient<TBase>::QueryInterface(riid, ppvObject);
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient2<TBase>::IsOffloadCapable(AUDIO_STREAM_CATEGORY Category, BOOL *pbOffloadCapable)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = this->m_RealAudioClient.IsOffloadCapable(Category, pbOffloadCapable);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient2<TBase>::SetClientProperties(const AudioClientProperties *pProperties)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = this->m_RealAudioClient.SetClientProperties(pProperties);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template<typename TBase>
HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient2<TBase>::GetBufferSizeLimits(const WAVEFORMATEX *pFormat, BOOL bEventDriven, REFERENCE_TIME *phnsMinBufferDuration, REFERENCE_TIME *phnsMaxBufferDuration)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = this->m_RealAudioClient.GetBufferSizeLimits(pFormat, bEventDriven, phnsMinBufferDuration, phnsMaxBufferDuration);
	DEBUG_PRINT_HR(hr);

	return hr;
}

template class DebugWrapperAudioClient2<IAudioClient2>;
template class DebugWrapperAudioClient2<IAudioClient3>;



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DebugWrapperAudioClient3::DebugWrapperAudioClient3(IAudioClient3& realAudioClient, const std::wstring deviceId)
	: DebugWrapperAudioClient2(realAudioClient, deviceId)
{
}

DebugWrapperAudioClient3::~DebugWrapperAudioClient3()
{
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient3::QueryInterface(REFIID riid, void **ppvObject)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << " riid: " << riid << std::endl;

	if (!ppvObject)
	{
		HRESULT hr = E_POINTER;
		DEBUG_PRINT_HR(hr);

		return hr;
	}

	if (riid == __uuidof(IAudioClient3))
	{
		*ppvObject = this;
		this->AddRef();
		return S_OK;
	}

	return DebugWrapperAudioClient2<IAudioClient3>::QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient3::GetSharedModeEnginePeriod(const WAVEFORMATEX *pFormat, UINT32 *pDefaultPeriodInFrames, UINT32 *pFundamentalPeriodInFrames, UINT32 *pMinPeriodInFrames, UINT32 *pMaxPeriodInFrames)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.GetSharedModeEnginePeriod(pFormat, pDefaultPeriodInFrames, pFundamentalPeriodInFrames, pMinPeriodInFrames, pMaxPeriodInFrames);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient3::GetCurrentSharedModeEnginePeriod(WAVEFORMATEX **ppFormat, UINT32 *pCurrentPeriodInFrames)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.GetCurrentSharedModeEnginePeriod(ppFormat, pCurrentPeriodInFrames);
	DEBUG_PRINT_HR(hr);

	return hr;
}

HRESULT STDMETHODCALLTYPE DebugWrapperAudioClient3::InitializeSharedAudioStream(DWORD StreamFlags, UINT32 PeriodInFrames, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid)
{
	rslog::info_ts() << this->GetDeviceId() << " " __FUNCTION__ << std::endl;

	HRESULT hr = m_RealAudioClient.InitializeSharedAudioStream(StreamFlags, PeriodInFrames, pFormat, AudioSessionGuid);
	DEBUG_PRINT_HR(hr);

	return hr;
}
