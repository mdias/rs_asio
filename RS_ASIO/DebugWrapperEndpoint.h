#pragma once

#include "ComBaseUnknown.h"

class DebugWrapperEndpoint : public ComBaseUnknown<IMMEndpoint>
{
public:
	DebugWrapperEndpoint(IMMEndpoint& realEndpoint, const std::wstring& deviceId);
	DebugWrapperEndpoint(const DebugWrapperEndpoint&) = delete;
	DebugWrapperEndpoint(DebugWrapperEndpoint&&) = delete;
	virtual ~DebugWrapperEndpoint();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

	virtual HRESULT STDMETHODCALLTYPE GetDataFlow(EDataFlow *pDataFlow) override;

private:
	IMMEndpoint& m_RealEndpoint;
	std::wstring m_DeviceId;
};