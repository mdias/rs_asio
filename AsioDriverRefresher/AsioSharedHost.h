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
	const bool GetIsAsio4All() const { return m_AsioDllIsAsio4all; }

	const std::string GetAsioDllPath() const { return m_AsioDllPath; }

	ASIOError SetSamplerate(const DWORD rate);
	ASIOError Setup(const WAVEFORMATEX& format, const DWORD bufferDurationFrames);
	void Reset();

	ASIOError Start();

	void Stop();
	bool GetPreferredBufferSize(DWORD& outBufferSizeFrames) const;
	bool ClampBufferSizeToLimits(DWORD& inOutBufferSizeFrames) const;

	void AddBufferSwitchListener(IAsioBufferSwitchListener* listener);
	void RemoveBufferSwitchListener(IAsioBufferSwitchListener* listener);

	bool IsWaveFormatSupported(const WAVEFORMATEX& format, bool output, unsigned firstAsioChannel, unsigned numAsioChannels) const;
	bool CheckSampleTypeAcrossChannels(ASIOSampleType& outType, bool output, unsigned firstAsioChannel, unsigned numAsioChannels) const;

	UINT32 GetBufferNumFrames() const;
	bool GetLatencyTime(REFERENCE_TIME& in, REFERENCE_TIME& out);

	ASIOBufferInfo* GetOutputBuffer(unsigned channel);
	ASIOBufferInfo* GetInputBuffer(unsigned channel);
	const ASIOChannelInfo* GetOutputChannelInfo(unsigned channel) const;
	const ASIOChannelInfo* GetInputChannelInfo(unsigned channel) const;
	unsigned GetNumInputChannels() const { return m_AsioInChannelInfo.size(); }
	unsigned GetNumOutputChannels() const { return m_AsioOutChannelInfo.size(); }

	void ResetDebugLogAsioBufferSwitches();

private:
	void DisplayCurrentError() const;

	void __cdecl AsioCalback_bufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	void __cdecl AsioCalback_sampleRateDidChange(ASIOSampleRate sRate);
	long __cdecl AsioCalback_asioMessage(long selector, long value, void* message, double* opt);
	ASIOTime* __cdecl AsioCalback_bufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);

	HMODULE m_Module = nullptr;
	IAsioDriver* m_Driver = nullptr;
	ULONG m_StartCount = 0;
	bool m_PostOutputReady = false;
	bool m_IsSetup = false;
	WAVEFORMATEXTENSIBLE m_CurrentWaveFormat;

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
	std::optional<ASIOSampleRate> m_SampleRateRestore;

	std::unique_ptr<std::thread> m_AsioEventsThread;

	unsigned m_dbgNumBufferSwitches;
	std::string m_DriverName;
	std::string m_AsioDllPath;
	bool m_AsioDllIsAsio4all = false;
};
