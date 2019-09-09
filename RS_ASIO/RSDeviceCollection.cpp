#include "stdafx.h"
#include "RSDeviceCollection.h"

RSDeviceCollection::RSDeviceCollection()
{
}

RSDeviceCollection::RSDeviceCollection(const RSDeviceCollection& other)
{
	Clear();
	m_Devices.reserve(other.m_Devices.size());
	for (IMMDevice* dev : other.m_Devices)
	{
		m_Devices.push_back(dev);
		dev->AddRef();
	}
}

RSDeviceCollection::RSDeviceCollection(RSDeviceCollection&& other)
{
	m_Devices.swap(other.m_Devices);
}

RSDeviceCollection::~RSDeviceCollection()
{
	Clear();
}

HRESULT STDMETHODCALLTYPE RSDeviceCollection::GetCount(UINT *pcDevices)
{
	if (!pcDevices)
		return E_POINTER;
	*pcDevices = (UINT)m_Devices.size();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE RSDeviceCollection::Item(UINT nDevice, IMMDevice **ppDevice)
{
	if (nDevice >= m_Devices.size())
		return E_INVALIDARG;
	if (!ppDevice)
		return E_POINTER;

	IMMDevice* dev = m_Devices[nDevice];
	dev->AddRef();
	*ppDevice = dev;

	return S_OK;
}

bool RSDeviceCollection::UpdateDevicesFromCollection(IMMDeviceCollection& collection, bool checkForRemoval)
{
	UINT n = 0;
	if (FAILED(collection.GetCount(&n)))
	{
		return false;
	}

	// extract devices into a vector to ease data iterating
	std::vector<IMMDevice*> devices;
	devices.reserve(n);

	for (UINT i = 0; i < n; ++i)
	{
		IMMDevice* dev = nullptr;
		if (SUCCEEDED(collection.Item(i, &dev)))
		{
			devices.push_back(dev);
		}
	}

	if (checkForRemoval)
	{
		RemoveIf([&devices](IMMDevice* dev) -> bool
			{
				return std::find(devices.begin(), devices.end(), dev) == devices.end();
			});
	}

	// add the new ones
	UINT numAdded = 0;
	for (IMMDevice* dev : devices)
	{
		if (!HasDevice(dev))
		{
			dev->AddRef();
			m_Devices.push_back(dev);
			++numAdded;
		}
	}

	// release temporary devices array
	for (IMMDevice* dev : devices)
	{
		dev->Release();
	}
	devices.clear();

	//rslog::info_ts() << "Added " << numAdded << " to collection; total now is " << m_Devices.size() << std::endl;

	return true;
}

bool RSDeviceCollection::HasDevice(IMMDevice* device) const
{
	return std::find(m_Devices.begin(), m_Devices.end(), device) != m_Devices.end();
}

void RSDeviceCollection::AddDevice(IMMDevice* device)
{
	if (device)
	{
		device->AddRef();
		m_Devices.push_back(device);
	}
}

void RSDeviceCollection::Clear()
{
	for (auto device : m_Devices)
	{
		device->Release();
	}
	m_Devices.clear();
}

void RSDeviceCollection::RemoveIf(std::function<bool(IMMDevice*)> callback)
{
	auto itRemove = std::remove_if(m_Devices.begin(), m_Devices.end(), callback);
	for (auto it = itRemove; it != m_Devices.end(); ++it)
	{
		(*it)->Release();
	}
	m_Devices.erase(itRemove, m_Devices.end());
}

IMMDevice* RSDeviceCollection::FindById(LPCWSTR id)
{
	for (IMMDevice* dev : m_Devices)
	{
		LPWSTR strId = nullptr;
		if (SUCCEEDED(dev->GetId(&strId)))
		{
			bool match = (wcscmp(id, strId) == 0);
			CoTaskMemFree(strId);
			if (match)
			{
				return dev;
			}
		}
	}

	return nullptr;
}