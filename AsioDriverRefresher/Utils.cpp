#include "stdafx.h"
#include <propsys.h>

DEFINE_PROPERTYKEY(PKEY_Device_DeviceIdHiddenKey1, 0xb3f8fa53, 0x0004, 0x438e, 0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b,
                   0xfc, 2);
DEFINE_PROPERTYKEY(PKEY_Device_DeviceIdHiddenKey2, 0x83DA6326, 0x97A6, 0x4088, 0x94, 0x53, 0xA1, 0x92, 0x3F, 0x57, 0x3B,
                   0x29, 3);

LONGLONG TimeStamp::GetMilisecs() const {
    LONGLONG freq = GetPerformanceFreq();
    if (!freq)
        return 0;

    return (perfCount * 1000) / freq;
}

LONGLONG TimeStamp::GetMicrosecs() const {
    LONGLONG freq = GetPerformanceFreq();
    if (!freq)
        return 0;

    return (perfCount * 1000) / (freq / 1000);
}

double TimeStamp::GetSeconds() const {
    LONGLONG microsecs = GetMicrosecs();
    LONGLONG secs = microsecs / 1000000;
    microsecs -= (secs * 1000000);

    return (double) secs + (((double) microsecs) / 1000000.0);
}

TimeStamp TimeStamp::operator -(const TimeStamp &other) {
    return TimeStamp(perfCount - other.perfCount);
}

LONGLONG TimeStamp::GetPerformanceFreq() const {
    static LARGE_INTEGER freq{};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    return freq.QuadPart;
}

std::string ConvertWStrToStr(const std::wstring &wstr) {
    typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;

    static const std::locale locale("");
    const converter_type &converter = std::use_facet<converter_type>(locale);

    std::vector<char> to(wstr.length() * converter.max_length());
    std::mbstate_t state;
    const wchar_t *from_next;
    char *to_next;
    const converter_type::result result = converter.out(state, wstr.data(), wstr.data() + wstr.length(), from_next,
                                                        &to[0], &to[0] + to.size(), to_next);

    if (result == converter_type::ok or result == converter_type::noconv) {
        return std::string(&to[0], to_next);
    }

    return std::string();
}

std::string IID2String(REFIID iid) {
    std::string ret;

    LPOLESTR iidStr = nullptr;
    StringFromIID(iid, &iidStr);

    if (iidStr) {
#if defined(_WIN32) && !defined(OLE2ANSI)
        std::wstring wStr = iidStr;
        ret = ConvertWStrToStr(wStr);
#else
		ret = iidStr;
#endif
        CoTaskMemFree(iidStr);
    }

    return ret;
}

const char *Dataflow2String(EDataFlow dataFlow) {
    if (dataFlow == eRender)
        return "eRender";
    else if (dataFlow == eCapture)
        return "eCapture";
    else if (dataFlow == eAll)
        return "eAll";
    return "N/A";
}

const char *Role2String(ERole role) {
    switch (role) {
        case eConsole: return "eConsole";
        case eMultimedia: return "eMultimedia";
        case eCommunications: return "eCommunications";
    }
    return "N/A";
}

std::ostream &operator<<(std::ostream &os, REFIID iid) {
    if (iid == __uuidof(IUnknown)) {
        os << "IID_IUnknown";
    } else if (iid == __uuidof(IMMDevice)) {
        os << "IID_IMMDevice";
    } else if (iid == __uuidof(IMMDeviceEnumerator)) {
        os << "IID_IMMDeviceEnumerator";
    } else if (iid == __uuidof(IAudioClient)) {
        os << "IID_IAudioClient";
    } else if (iid == __uuidof(IAudioClient2)) {
        os << "IID_IAudioClient2";
    } else if (iid == __uuidof(IAudioClient3)) {
        os << "IID_IAudioClient3";
    } else if (iid == __uuidof(IAudioRenderClient)) {
        os << "IID_IAudioRenderClient";
    } else if (iid == __uuidof(IAudioCaptureClient)) {
        os << "IID_IAudioCaptureClient";
    } else if (iid == __uuidof(IMMEndpoint)) {
        os << "IID_IMMEndpoint";
    } else if (iid == __uuidof(IAudioEndpointVolume)) {
        os << "IID_IAudioEndpointVolume";
    } else {
        LPOLESTR clsidStr = nullptr;
        if (FAILED(StringFromCLSID(iid, &clsidStr))) {
            os << "{N/A}";
            return os;
        }

        os << clsidStr;

        CoTaskMemFree(clsidStr);
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, REFPROPERTYKEY key) {
    if (key == PKEY_AudioEngine_DeviceFormat) {
        os << "PKEY_AudioEngine_DeviceFormat";
    } else if (key == PKEY_Device_FriendlyName) {
        os << "PKEY_Device_FriendlyName";
    } else if (key == PKEY_Device_DeviceIdHiddenKey1) {
        os << "PKEY_Device_DeviceIdHiddenKey1";
    } else if (key == PKEY_Device_DeviceIdHiddenKey2) {
        os << "PKEY_Device_DeviceIdHiddenKey2";
    } else if (key == PKEY_AudioEndpoint_FormFactor) {
        os << "PKEY_AudioEndpoint_FormFactor";
    } else {
        wchar_t propNameW[128];
        HRESULT hr = PSStringFromPropertyKey(key, propNameW, 128);
        if (SUCCEEDED(hr)) {
            os << propNameW;
        } else {
            os << "N/A";
        }
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const wchar_t *wStr) {
    std::string propName = ConvertWStrToStr(wStr);
    os << propName.c_str();
    return os;
}

std::ostream &operator<<(std::ostream &os, const std::wstring wStr) {
    return os << wStr.c_str();
}

std::ostream &operator<<(std::ostream &os, const WAVEFORMATEX &fmt) {
    os << "WAVEFORMATEX\n";
    os << "  wFormatTag: " << std::hex << fmt.wFormatTag << std::endl;
    os << "  nChannels: " << std::dec << fmt.nChannels << std::endl;
    os << "  nSamplesPerSec: " << std::dec << fmt.nSamplesPerSec << std::endl;
    os << "  nAvgBytesPerSec: " << std::dec << fmt.nAvgBytesPerSec << std::endl;
    os << "  nBlockAlign: " << std::dec << fmt.nBlockAlign << std::endl;
    os << "  wBitsPerSample: " << std::dec << fmt.wBitsPerSample << std::endl;
    os << "  cbSize: " << std::dec << fmt.cbSize << std::endl;

    if (fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const WAVEFORMATEXTENSIBLE &ext = (WAVEFORMATEXTENSIBLE &) fmt;
        os << "  ext.SubFormat: ";

        if (ext.SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
            os << "KSDATAFORMAT_SUBTYPE_PCM" << std::endl;
        else if (ext.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            os << "KSDATAFORMAT_SUBTYPE_IEEE_FLOAT" << std::endl;
        else
            os << std::hex << ext.SubFormat << std::endl;

        os << "  ext.Samples: " << std::dec << ext.Samples.wSamplesPerBlock << std::endl;
        os << "  ext.dwChannelMask: " << std::hex << ext.dwChannelMask << std::endl;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const AUDCLNT_SHAREMODE &mode) {
    switch (mode) {
        case AUDCLNT_SHAREMODE_SHARED:
            os << "Shared";
            break;
        case AUDCLNT_SHAREMODE_EXCLUSIVE:
            os << "Exclusive";
            break;
        default:
            os << "?";
            break;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const HResultToStr &hresult) {
#define CASE_TO_STR(x) case x: os << #x; break

    switch (hresult.hr) {
        CASE_TO_STR(S_OK);
        CASE_TO_STR(S_FALSE);

        CASE_TO_STR(E_FAIL);
        CASE_TO_STR(E_POINTER);
        CASE_TO_STR(E_INVALIDARG);
        CASE_TO_STR(E_OUTOFMEMORY);
        CASE_TO_STR(E_NOTIMPL);

        CASE_TO_STR(AUDCLNT_E_ALREADY_INITIALIZED);
        CASE_TO_STR(AUDCLNT_E_WRONG_ENDPOINT_TYPE);
        CASE_TO_STR(AUDCLNT_E_BUFFER_ERROR);
        CASE_TO_STR(AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED);
        CASE_TO_STR(AUDCLNT_E_BUFFER_SIZE_ERROR);
        CASE_TO_STR(AUDCLNT_E_CPUUSAGE_EXCEEDED);
        CASE_TO_STR(AUDCLNT_E_DEVICE_INVALIDATED);
        CASE_TO_STR(AUDCLNT_E_DEVICE_IN_USE);
        CASE_TO_STR(AUDCLNT_E_ENDPOINT_CREATE_FAILED);
        CASE_TO_STR(AUDCLNT_E_INVALID_DEVICE_PERIOD);
        CASE_TO_STR(AUDCLNT_E_UNSUPPORTED_FORMAT);
        CASE_TO_STR(AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED);
        CASE_TO_STR(AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL);
        CASE_TO_STR(AUDCLNT_E_SERVICE_NOT_RUNNING);
        CASE_TO_STR(AUDCLNT_E_EVENTHANDLE_NOT_SET);
        CASE_TO_STR(AUDCLNT_E_NOT_STOPPED);
        default:
            os << std::hex << hresult.hr;
            break;
    }

#undef CASE_TO_STR

    return os;
}

std::ostream &operator<<(std::ostream &os, ASIOSampleType sampleType) {
#define CASE_TO_STR(x) case x: os << #x; break

    switch (sampleType) {
        CASE_TO_STR(ASIOSTInt16MSB);
        CASE_TO_STR(ASIOSTInt24MSB);
        CASE_TO_STR(ASIOSTInt32MSB);
        CASE_TO_STR(ASIOSTFloat32MSB);
        CASE_TO_STR(ASIOSTFloat64MSB);
        CASE_TO_STR(ASIOSTInt32MSB16);
        CASE_TO_STR(ASIOSTInt32MSB18);
        CASE_TO_STR(ASIOSTInt32MSB20);
        CASE_TO_STR(ASIOSTInt32MSB24);
        CASE_TO_STR(ASIOSTInt16LSB);
        CASE_TO_STR(ASIOSTInt24LSB);
        CASE_TO_STR(ASIOSTInt32LSB);
        CASE_TO_STR(ASIOSTFloat32LSB);
        CASE_TO_STR(ASIOSTFloat64LSB);
        CASE_TO_STR(ASIOSTInt32LSB16);
        CASE_TO_STR(ASIOSTInt32LSB18);
        CASE_TO_STR(ASIOSTInt32LSB20);
        CASE_TO_STR(ASIOSTInt32LSB24);
        CASE_TO_STR(ASIOSTDSDInt8LSB1);
        CASE_TO_STR(ASIOSTDSDInt8MSB1);
        CASE_TO_STR(ASIOSTDSDInt8NER8);
        default:
            os << std::dec << (long) sampleType;
            break;
    }

#undef CASE_TO_STR

    return os;
}

std::ostream &operator<<(std::ostream &os, const TimeStamp &time) {
    return os << time.GetSeconds();
}

REFERENCE_TIME MilisecsToRefTime(LONGLONG ms) {
    return ms * 10000;
}

LONGLONG RefTimeToMilisecs(const REFERENCE_TIME &time) {
    return time / 10000;
}

LONGLONG DurationToAudioFrames(const REFERENCE_TIME &time, DWORD sampleRate) {
    return (time * sampleRate) / 10000000;
}

REFERENCE_TIME AudioFramesToDuration(const LONGLONG &frames, DWORD sampleRate) {
    if (sampleRate == 0)
        return 0;
    return (frames * 10000000) / sampleRate;
}

bool AsioSampleTypeFromFormat(ASIOSampleType *out, WORD bitsPerSample, bool isFloat) {
    bool ret = true;
    ASIOSampleType type = ASIOSTInt32LSB;

    if (!isFloat) {
        switch (bitsPerSample) {
            case 16:
                type = ASIOSTInt16LSB;
                break;
            case 24:
                type = ASIOSTInt24LSB;
                break;
            case 32:
                type = ASIOSTInt32LSB;
                break;
            default:
                return false;
        }
    } else {
        switch (bitsPerSample) {
            case 32:
                type = ASIOSTFloat32LSB;
                break;
            case 64:
                type = ASIOSTFloat64LSB;
                break;
            default:
                return false;
        }
    }

    if (out)
        *out = type;

    return ret;
}

WORD GetAsioSampleTypeNumBytes(ASIOSampleType sampleType) {
    switch (sampleType) {
        case ASIOSTInt16LSB:
            return 2;
        case ASIOSTInt24LSB:
            return 3;
        case ASIOSTInt32LSB:
        case ASIOSTFloat32LSB:
            return 4;
        case ASIOSTFloat64LSB:
            return 8;
    }

    return 0;
}

bool IsWaveFormatSame(const WAVEFORMATEX &fmt_a, const WAVEFORMATEX &fmt_b) {
    if (fmt_a.wFormatTag != fmt_b.wFormatTag ||
        fmt_a.nChannels != fmt_b.nChannels ||
        fmt_a.nSamplesPerSec != fmt_b.nSamplesPerSec ||
        fmt_a.nAvgBytesPerSec != fmt_b.nAvgBytesPerSec ||
        fmt_a.nBlockAlign != fmt_b.nBlockAlign ||
        fmt_a.wBitsPerSample != fmt_b.wBitsPerSample ||
        fmt_a.cbSize != fmt_b.cbSize) {
        return false;
    }

    if (fmt_a.wFormatTag == WAVE_FORMAT_EXTENSIBLE && fmt_a.cbSize >= sizeof(WAVEFORMATEXTENSIBLE)) {
        const WAVEFORMATEXTENSIBLE &exfmt_a = (const WAVEFORMATEXTENSIBLE &) fmt_a;
        const WAVEFORMATEXTENSIBLE &exfmt_b = (const WAVEFORMATEXTENSIBLE &) fmt_b;

        if (exfmt_a.Samples.wValidBitsPerSample != exfmt_a.Samples.wValidBitsPerSample ||
            exfmt_a.dwChannelMask != exfmt_a.dwChannelMask ||
            exfmt_a.SubFormat != exfmt_a.SubFormat) {
            return false;
        }
    }

    return true;
}

HWND GetGameWindow() {
    return FindWindowA("Rocksmith 2014", "Rocksmith 2014");
}

std::string GetTimestamp() {
    // Get current time using chrono library
    auto now = std::chrono::system_clock::now();

    // Convert to time_t (standard time representation)
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // Get milliseconds since epoch (epoch is Jan 1, 1970)
    auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // Convert to std::tm structure for formatting
    std::tm now_tm;
    localtime_s(&now_tm, &now_time_t); // Windows specific, use localtime_r on Unix-like systems

    // Format the date/time string including milliseconds
    char buffer[80]; // Buffer to hold the formatted date/time string
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &now_tm); // Format up to seconds

    // Append milliseconds to the formatted string
    std::string datetime_with_ms = buffer;
    datetime_with_ms += '.' + std::to_string(ms_since_epoch % 1000);

    return datetime_with_ms;
}

std::string trimString(const std::string &s) {
    if (s.empty())
        return s;

    const char p[] = " \t\r\n";
    size_t start = 0;
    size_t end = s.length() - 1;

    // find left position to trim
    for (; start < end; ++start) {
        bool skip = false;
        for (int i = 0; i < sizeof(p); ++i) {
            if (s[start] == p[i]) {
                skip = true;
                break;
            }
        }
        if (!skip)
            break;
    }

    // find right position to trim
    for (; end > start && end >= 1; --end) {
        bool skip = false;
        for (int i = 0; i < sizeof(p); ++i) {
            if (s[end] == p[i]) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            break;
        }
    }

    return s.substr(start, (end + 1) - start);
}

std::string toLowerString(const std::string s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
    return res;
}
