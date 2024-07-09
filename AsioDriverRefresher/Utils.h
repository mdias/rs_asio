#pragma once

EXTERN_C const PROPERTYKEY PKEY_Device_DeviceIdHiddenKey1;
EXTERN_C const PROPERTYKEY PKEY_Device_DeviceIdHiddenKey2;

struct HResultToStr {
    HResultToStr(HRESULT hr) { this->hr = hr; }
    HRESULT hr;
};

struct TimeStamp {
    TimeStamp() {
        QueryPerformanceCounter((LARGE_INTEGER *) &perfCount);
    }

    TimeStamp(LONGLONG pc) : perfCount(pc) {
    }

    LONGLONG GetMilisecs() const;

    LONGLONG GetMicrosecs() const;

    double GetSeconds() const;

    TimeStamp operator -(const TimeStamp &other);

    LONGLONG perfCount = 0;

private:
    LONGLONG GetPerformanceFreq() const;
};

std::string ConvertWStrToStr(const std::wstring &wstr);

std::string trimString(const std::string &s);

std::string toLowerString(std::string s);

std::string IID2String(REFIID iid);

const char *Dataflow2String(EDataFlow dataFlow);

const char *Role2String(ERole role);

std::ostream &operator<<(std::ostream &os, REFIID iid);

std::ostream &operator<<(std::ostream &os, REFPROPERTYKEY key);

std::ostream &operator<<(std::ostream &os, const wchar_t *wStr);

std::ostream &operator<<(std::ostream &os, const std::wstring wStr);

std::ostream &operator<<(std::ostream &os, const WAVEFORMATEX &fmt);

std::ostream &operator<<(std::ostream &os, const AUDCLNT_SHAREMODE &mode);

std::ostream &operator<<(std::ostream &os, const HResultToStr &hresult);

std::ostream &operator<<(std::ostream &os, ASIOSampleType sampleType);

std::ostream &operator<<(std::ostream &os, const TimeStamp &time);


std::string GetTimestamp();

REFERENCE_TIME MilisecsToRefTime(LONGLONG ms);

LONGLONG RefTimeToMilisecs(const REFERENCE_TIME &time);

LONGLONG DurationToAudioFrames(const REFERENCE_TIME &time, DWORD sampleRate);

REFERENCE_TIME AudioFramesToDuration(const LONGLONG &frames, DWORD sampleRate);

bool AsioSampleTypeFromFormat(ASIOSampleType *out, WORD bitsPerSample, bool isFloat);

WORD GetAsioSampleTypeNumBytes(ASIOSampleType sampleType);

bool IsWaveFormatSame(const WAVEFORMATEX &fmt_a, const WAVEFORMATEX &fmt_b);

HWND GetGameWindow();
