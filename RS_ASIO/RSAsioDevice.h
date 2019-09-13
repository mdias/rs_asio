#pragma once

#include "ComBaseUnknown.h"
#include "AsioSharedHost.h"

class RSAsioDevice : public ComBaseUnknown<IMMDevice>
{
public:
	struct Config
	{
		bool isOutput;
		unsigned baseAsioChannelNumber = 0;
		unsigned numAsioChannels = 1;
		AsioSharedHost::BufferSizeMode bufferSizeMode = AsioSharedHost::BufferSizeMode_Driver;
	};

public:
	RSAsioDevice(AsioSharedHost& asioSharedHost, const std::wstring& id, const Config& config);
	RSAsioDevice(const RSAsioDevice&) = delete;
	RSAsioDevice(RSAsioDevice&&) = delete;
	virtual ~RSAsioDevice();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

	virtual HRESULT STDMETHODCALLTYPE Activate(REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface) override;
	virtual HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD stgmAccess, IPropertyStore **ppProperties) override;
	virtual HRESULT STDMETHODCALLTYPE GetId(LPWSTR *ppstrId) override;
	virtual HRESULT STDMETHODCALLTYPE GetState(DWORD *pdwState) override;

	AsioSharedHost& GetAsioHost();
	const Config& GetConfig() const { return m_Config; }

	unsigned GetNumWasapiChannels() const;

	const std::wstring& GetIdRef() const { return m_Id; }

private:
	std::wstring m_Id;
	Config m_Config;

	AsioSharedHost& m_AsioSharedHost;
	unsigned m_BaseChannelNumber;
	unsigned m_NumChannels;
};

class RSAsioEndpoint : public ComBaseUnknown<IMMEndpoint>
{
public:
	RSAsioEndpoint(RSAsioDevice& asioDevice, EDataFlow dataFlow);
	RSAsioEndpoint(const RSAsioEndpoint&) = delete;
	RSAsioEndpoint(RSAsioEndpoint&&) = delete;
	virtual ~RSAsioEndpoint();

	virtual HRESULT STDMETHODCALLTYPE GetDataFlow(EDataFlow *pDataFlow);

private:
	RSAsioDevice& m_AsioDevice;
	EDataFlow m_DataFlow;
};