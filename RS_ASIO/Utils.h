#pragma once

EXTERN_C const PROPERTYKEY PKEY_Device_DeviceIdHiddenKey1;
EXTERN_C const PROPERTYKEY PKEY_Device_DeviceIdHiddenKey2;

struct HResultToStr
{
	HResultToStr(HRESULT hr) { this->hr = hr; }
	HRESULT hr;
};

std::string ConvertWStrToStr(const std::wstring& wstr);
std::string IID2String(REFIID iid);
const char* Dataflow2String(EDataFlow dataFlow);
const char* Role2String(ERole role);

std::ostream & operator<<(std::ostream & os, REFIID iid);
std::ostream & operator<<(std::ostream & os, REFPROPERTYKEY key);
std::ostream & operator<<(std::ostream & os, const wchar_t* wStr);
std::ostream & operator<<(std::ostream & os, const std::wstring wStr);
std::ostream & operator<<(std::ostream & os, const WAVEFORMATEX& fmt);
std::ostream & operator<<(std::ostream & os, const AUDCLNT_SHAREMODE& mode);
std::ostream & operator<<(std::ostream & os, const HResultToStr& hresult);
std::ostream & operator<<(std::ostream & os, ASIOSampleType sampleType);

REFERENCE_TIME MilisecsToRefTime(LONGLONG ms);
LONGLONG RefTimeToMilisecs(const REFERENCE_TIME& time);
LONGLONG DurationToAudioFrames(const REFERENCE_TIME& time, DWORD sampleRate);
REFERENCE_TIME AudioFramesToDuration(const LONGLONG& frames, DWORD sampleRate);
