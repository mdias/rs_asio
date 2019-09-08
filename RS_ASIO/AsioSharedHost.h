#pragma once

#include "ComBaseUnknown.h"

class IAsioBufferSwitchListener
{
public:
	virtual void OnAsioBufferSwitch(unsigned buffIdx) = 0;
};

class AsioSharedHost : public ComBaseUnknown<IUnknown>
{
public:
	AsioSharedHost(const CLSID& clsid, const std::string& asioDllPath);
	AsioSharedHost(const AsioSharedHost&) = delete;
	AsioSharedHost(AsioSharedHost&&) = delete;
	virtual ~AsioSharedHost();

	bool IsValid() const;
	IAsioDriver* GetDriver() { return m_Driver; }

	ASIOError Start(const WAVEFORMATEX& format, const REFERENCE_TIME& suggestedBufferDuration);
	void Stop();

	void AddBufferSwitchListener(IAsioBufferSwitchListener* listener);
	void RemoveBufferSwitchListener(IAsioBufferSwitchListener* listener);

	bool IsWaveFormatSupported(const WAVEFORMATEX& format, bool output, unsigned firstAsioChannel, unsigned numAsioChannels) const;
	bool CheckSampleTypeAcrossChannels(ASIOSampleType& outType, bool output, unsigned firstAsioChannel, unsigned numAsioChannels) const;

	UINT32 GetBufferNumFrames() const;
	bool GetLatencyTime(REFERENCE_TIME& in, REFERENCE_TIME& out);

	ASIOBufferInfo* GetOutputBuffer(unsigned channel);
	ASIOBufferInfo* GetInputBuffer(unsigned channel);
	unsigned GetNumInputChannels() const { return m_AsioInChannelInfo.size(); }
	unsigned GetNumOutputChannels() const { return m_AsioOutChannelInfo.size(); }
public:
	void DisplayCurrentError();

	void __cdecl AsioCalback_bufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	void __cdecl AsioCalback_sampleRateDidChange(ASIOSampleRate sRate);
	long __cdecl AsioCalback_asioMessage(long selector, long value, void* message, double* opt);
	ASIOTime* __cdecl AsioCalback_bufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);

	HMODULE m_Module = nullptr;
	IAsioDriver* m_Driver = nullptr;
	ULONG m_StartCount = 0;
	WAVEFORMATEX m_CurrentWaveFormat;

	TrampolineToMethod<decltype(ASIOCallbacks::bufferSwitch)> m_Trampoline_bufferSwitch;
	TrampolineToMethod<decltype(ASIOCallbacks::sampleRateDidChange)> m_Trampoline_sampleRateDidChange;
	TrampolineToMethod<decltype(ASIOCallbacks::asioMessage)> m_Trampoline_asioMessage;
	TrampolineToMethod<decltype(ASIOCallbacks::bufferSwitchTimeInfo)> m_Trampoline_bufferSwitchTimeInfo;
	ASIOCallbacks m_AsioCallbacks;

	UINT32 m_NumBufferFrames = 0;
	std::vector<ASIOBufferInfo> m_AsioBuffers;
	std::vector<ASIOChannelInfo> m_AsioInChannelInfo;
	std::vector<ASIOChannelInfo> m_AsioOutChannelInfo;
	std::set<IAsioBufferSwitchListener*> m_AsioBufferListeners;
	std::mutex m_AsioMutex;

	std::unique_ptr<std::thread> m_AsioEventsThread;
};
