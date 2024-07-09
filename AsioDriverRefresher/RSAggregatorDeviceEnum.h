#pragma once

#include "RSBaseDeviceEnum.h"

class RSAggregatorDeviceEnum : public RSBaseDeviceEnum
{
public:
	RSAggregatorDeviceEnum();
	RSAggregatorDeviceEnum(const RSAggregatorDeviceEnum&) = delete;
	RSAggregatorDeviceEnum(RSAggregatorDeviceEnum&&) = delete;
	virtual ~RSAggregatorDeviceEnum();

	void AddDeviceEnumerator(IMMDeviceEnumerator* enumerator, bool enableOutputs, bool enableInputs);

protected:
	struct TrackedEnumerator
	{
		TrackedEnumerator(IMMDeviceEnumerator* enumerator, bool enableOutputs, bool enableInputs)
		{
			this->enumerator = enumerator;
			this->enableOutputs = enableOutputs;
			this->enableInputs = enableInputs;
			if (enumerator)
			{
				enumerator->AddRef();
			}
		}

		TrackedEnumerator(const TrackedEnumerator& other)
		{
			*this = other;
		}

		TrackedEnumerator(TrackedEnumerator&& other)
		{
			*this = other;
			other.enumerator = nullptr;
		}

		~TrackedEnumerator()
		{
			if (enumerator)
			{
				enumerator->Release();
			}
		}

		TrackedEnumerator& operator=(const TrackedEnumerator& other)
		{
			enumerator = other.enumerator;
			enableOutputs = other.enableOutputs;
			enableInputs = other.enableInputs;
			if (enumerator)
			{
				enumerator->AddRef();
			}
			return *this;
		}

		IMMDeviceEnumerator* enumerator = nullptr;
		bool enableOutputs = true;
		bool enableInputs = true;
	};

protected:
	TrackedEnumerator* FindTrackedEnumerator(IMMDeviceEnumerator* enumerator);
	virtual void UpdateAvailableDevices() override;
	virtual void ClearAll() override;

	std::vector<TrackedEnumerator> m_Enumerators;
};
