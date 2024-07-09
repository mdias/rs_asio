#pragma once

#include "ComBaseUnknown.h"
#include <memory>
#include <thread>

class IAsioBufferSwitchListener {
public:
    virtual void OnAsioBufferSwitch(unsigned buffIdx) = 0;
};

class AsioSharedHost : public ComBaseUnknown<IUnknown> {
public:
    AsioSharedHost(const CLSID &clsid, const std::string &asioDllPath);

    AsioSharedHost(const AsioSharedHost &) = delete;

    AsioSharedHost(AsioSharedHost &&) = delete;

    virtual ~AsioSharedHost();

    bool IsValid() const;

    IAsioDriver *GetDriver() { return m_Driver; }
    const bool GetIsAsio4All() const { return m_AsioDllIsAsio4all; }

    const std::string GetAsioDllPath() const { return m_AsioDllPath; }

    ASIOError SetBufferSize(DWORD bufferDurationFrames, bool backup);

    ASIOError Refresh();

    void Backup() const;

    void Restore();

    ASIOError Set(long buffer);

    static long IntermediateBufferSize(long minAsioBufferFrames, long maxAsioBufferFrames, long currentBufferFrames);

    ASIOError Init();

    void Reset();

    long getCurrentBufferSize() const;

    ASIOError Start();

    void Stop();

    bool GetPreferredBufferSize(DWORD &outBufferSizeFrames) const;

    bool ClampBufferSizeToLimits(DWORD &inOutBufferSizeFrames) const;

    void AddBufferSwitchListener(IAsioBufferSwitchListener *listener);

    void RemoveBufferSwitchListener(IAsioBufferSwitchListener *listener);

    bool IsWaveFormatSupported(const WAVEFORMATEX &format, bool output, unsigned firstAsioChannel,
                               unsigned numAsioChannels) const;

    bool CheckSampleTypeAcrossChannels(ASIOSampleType &outType, bool output, unsigned firstAsioChannel,
                                       unsigned numAsioChannels) const;

    UINT32 GetBufferNumFrames() const;

    bool GetLatencyTime(REFERENCE_TIME &in, REFERENCE_TIME &out);

    ASIOBufferInfo *GetOutputBuffer(unsigned channel);

    ASIOBufferInfo *GetInputBuffer(unsigned channel);

    const ASIOChannelInfo *GetOutputChannelInfo(unsigned channel) const;

    const ASIOChannelInfo *GetInputChannelInfo(unsigned channel) const;

    unsigned GetNumInputChannels() const { return m_AsioInChannelInfo.size(); }
    unsigned GetNumOutputChannels() const { return m_AsioOutChannelInfo.size(); }

    void ResetDebugLogAsioBufferSwitches();

private:
    void DisplayCurrentError() const;

    void __cdecl AsioCalback_bufferSwitch(long doubleBufferIndex, ASIOBool directProcess);

    void __cdecl AsioCalback_sampleRateDidChange(ASIOSampleRate sRate);

    long __cdecl AsioCalback_asioMessage(long selector, long value, void *message, double *opt);

    ASIOTime * __cdecl AsioCalback_bufferSwitchTimeInfo(ASIOTime *params, long doubleBufferIndex,
                                                        ASIOBool directProcess);

    HMODULE m_Module = nullptr;
    IAsioDriver *m_Driver = nullptr;
    ULONG m_StartCount = 0;
    bool m_PostOutputReady = false;
    bool m_IsSetup = false;
    WAVEFORMATEXTENSIBLE m_CurrentWaveFormat{};
    long backupBufferSize = 0;

    ASIOCallbacks m_AsioCallbacks{};

    UINT32 m_NumBufferFrames = 0;
    std::vector<ASIOBufferInfo> m_AsioBuffers;
    std::vector<ASIOChannelInfo> m_AsioInChannelInfo;
    std::vector<ASIOChannelInfo> m_AsioOutChannelInfo;
    std::set<IAsioBufferSwitchListener *> m_AsioBufferListeners;
    std::mutex m_AsioMutex;
    std::optional<ASIOSampleRate> m_SampleRateRestore;

    std::unique_ptr<std::thread> m_AsioEventsThread;

    unsigned m_dbgNumBufferSwitches{};
    std::string m_DriverName;
    std::string m_AsioDllPath;
    bool m_AsioDllIsAsio4all = false;
};
