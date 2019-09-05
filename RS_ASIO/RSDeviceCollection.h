#pragma once

#include "ComBaseUnknown.h"

class RSDeviceCollection : public ComBaseUnknown<IMMDeviceCollection>
{
public:
	RSDeviceCollection();
	RSDeviceCollection(const RSDeviceCollection&);
	RSDeviceCollection(RSDeviceCollection&&);
	virtual ~RSDeviceCollection();

	virtual HRESULT STDMETHODCALLTYPE GetCount(UINT *pcDevices) override;
	virtual HRESULT STDMETHODCALLTYPE Item(UINT nDevice, IMMDevice **ppDevice) override;

	bool UpdateDevicesFromCollection(IMMDeviceCollection& collection, bool checkForRemoval);
	bool HasDevice(IMMDevice* device) const;
	void AddDevice(IMMDevice* device);
	void Clear();
	void RemoveIf(std::function<bool(IMMDevice*)> callback);
	IMMDevice* FindById(LPCWSTR id);

	std::vector<IMMDevice*>::iterator begin() { return m_Devices.begin(); }
	std::vector<IMMDevice*>::iterator end() { return m_Devices.end(); }
	std::vector<IMMDevice*>::const_iterator cbegin() const { return m_Devices.cbegin(); }
	std::vector<IMMDevice*>::const_iterator cend() const { return m_Devices.cend(); }
	size_t size() const { return m_Devices.size(); }

private:
	ULONG m_RefCount = 1;
	std::vector<IMMDevice*> m_Devices;
};
