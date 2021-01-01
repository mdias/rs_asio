#include "stdafx.h"
#include "RSAggregatorDeviceEnum.h"
#include "RSAsioDeviceEnum.h"
#include "Configurator.h"
#include "AsioHelpers.h"
#include "AsioSharedHost.h"

static void LoadConfigIni(RSConfig& out);

static RSConfig& GetConfig()
{
	static RSConfig config;
	static bool configLoaded = false;
	if (!configLoaded)
	{
		configLoaded = true;
		LoadConfigIni(config);
	}

	return config;
}

static void AddWasapiDevices(RSAggregatorDeviceEnum& rsEnum, bool enableOutputs, bool enableInputs)
{
	if (!enableOutputs && !enableInputs)
		return;

	IMMDeviceEnumerator* wasapiEnum = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&wasapiEnum);
	if (SUCCEEDED(hr) && wasapiEnum)
	{
		rsEnum.AddDeviceEnumerator(wasapiEnum, enableOutputs, enableInputs);
		wasapiEnum->Release();
	}
}

static void AddAsioDevices(RSAggregatorDeviceEnum& rsEnum)
{
	auto asioEnum = new RSAsioDeviceEnum();
	asioEnum->SetConfig(GetConfig().asioConfig);

	rsEnum.AddDeviceEnumerator(asioEnum, true, true);
	asioEnum->Release();
}

void SetupDeviceEnumerator(RSAggregatorDeviceEnum& rsEnum)
{
	const RSConfig& config = GetConfig();

	if (config.enableAsio)
	{
		AddAsioDevices(rsEnum);
	}
	AddWasapiDevices(rsEnum, config.enableWasapiOutputs, config.enableWasapiInputs);
}

static const std::wstring& GetConfigFilePath()
{
	static std::wstring path;
	static bool pathComputed = false;

	if (!pathComputed)
	{
		wchar_t exePath[MAX_PATH]{};
		DWORD exePathSize = GetModuleFileNameW(NULL, exePath, MAX_PATH);
		if (exePathSize > 0)
		{
			int slashPos = -1;
			// find last slash and truncate the path there
			for (int i = (int)exePathSize - 1; i >= 0; --i)
			{
				const wchar_t c = exePath[i];
				if (c == '\\' || c == '/')
				{
					slashPos = i;
					break;
				}
			}

			if (slashPos > 0)
			{
				exePath[slashPos + 1] = '\0';
				path = exePath;
				path += L"RS_ASIO.ini";
			}
		}
	}

	return path;
}

static std::string trimString(const std::string& s)
{
	if (s.empty())
		return s;

	const char p[] = " \t\r\n";
	size_t start = 0;
	size_t end = s.length() - 1;

	// find left position to trim
	for (; start < end; ++start)
	{
		bool skip = false;
		for (int i = 0; i < sizeof(p); ++i)
		{
			if (s[start] == p[i])
			{
				skip = true;
				break;
			}
		}
		if (!skip)
			break;
	}

	// find right position to trim
	for (; end > start && end >=1; --end)
	{
		bool skip = false;
		for (int i = 0; i < sizeof(p); ++i)
		{
			if (s[end] == p[i])
			{
				skip = true;
				break;
			}
		}
		if (!skip)
		{
			break;
		}
	}

	return s.substr(start, (end+1) - start);
}

static std::string toLowerString(const std::string s)
{
	std::string res = s;
	std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
	return res;
}

static bool parseBoolString(const std::string& s, bool& out)
{
	if (s.empty())
		return false;

	if (s == "1")
	{
		out = true;
		return true;
	}
	if (s == "0")
	{
		out = false;
		return true;
	}

	std::string sl = toLowerString(s);
	if (sl == "true" || sl == "yes")
	{
		out = true;
		return true;
	}
	if (sl == "false" || sl == "no")
	{
		out = false;
		return true;
	}

	return false;
}

static bool parseIntString(const std::string& s, int& out)
{
	if (s.empty())
		return false;

	try
	{
		out = std::stoi(s);
	}
	catch (...)
	{
		return false;
	}

	return true;
}

static void LoadConfigIni(RSConfig& out)
{
	const std::wstring& cfgPath = GetConfigFilePath();
	if (cfgPath.size() == 0)
		return;

	std::ifstream file;
	file.open(cfgPath, std::ifstream::in);
	if (!file.is_open())
	{
		rslog::info_ts() << "failed to open config file" << std::endl;
		return;
	}

	enum
	{
		SectionNone,
		SectionConfig,
		SectionAsio,
		SectionAsioOut,
		SectionAsioIn0,
		SectionAsioIn1,
		SectionAsioInMic,
	} currentSection = SectionNone;

	bool isOldWasapiCfgMode = true;

	std::string currentLine;
	size_t line = 0;
	while (std::getline(file, currentLine))
	{
		++line;
		currentLine = trimString(currentLine);
		if (currentLine.empty())
			continue;

		// new section
		if (currentLine[0] == '[')
		{
			size_t pos = currentLine.find(']');
			if (pos != (currentLine.length()-1))
			{
				rslog::error_ts() << __FUNCTION__ << " - malformed ini section found at line " << line << std::endl;
			}
			else
			{
				std::string sectionName = toLowerString(currentLine.substr(1, currentLine.length() - 2));
				if (sectionName == "config")
					currentSection = SectionConfig;
				else if (sectionName == "asio")
					currentSection = SectionAsio;
				else if (sectionName == "asio.output")
					currentSection = SectionAsioOut;
				else if (sectionName == "asio.input.0")
					currentSection = SectionAsioIn0;
				else if (sectionName == "asio.input.1")
					currentSection = SectionAsioIn1;
				else if (sectionName == "asio.input.mic")
					currentSection = SectionAsioInMic;
			}
		}
		else if (currentLine[0] == ';' || currentLine[0] == '#')
		{}
		else
		{
			// not even worth checking what this is until we're in a section
			if (currentSection == SectionNone)
				continue;

			std::string key, val;

			const size_t posEqual = currentLine.find('=');
			if (posEqual != std::string::npos)
			{
				key = toLowerString(currentLine.substr(0, posEqual));
				val = trimString(currentLine.substr(posEqual + 1));
			}

			if (!key.empty() && !val.empty())
			{
				if (currentSection == SectionConfig)
				{
					if (key == "enablewasapioutputs")
					{
						isOldWasapiCfgMode = false;
						parseBoolString(val, out.enableWasapiOutputs);
					}
					else if (key == "enablewasapiinputs")
					{
						isOldWasapiCfgMode = false;
						parseBoolString(val, out.enableWasapiInputs);
					}
					else if (key == "enablewasapi")
					{
						if (isOldWasapiCfgMode)
						{
							bool v = false;
							parseBoolString(val, v);
							out.enableWasapiOutputs = out.enableWasapiInputs = v;
						}
					}
					else if (key == "enableasio")
						parseBoolString(val, out.enableAsio);
				}
				else if (currentSection == SectionAsio)
				{
					if (key == "buffersizemode")
					{
						const std::string valLower = toLowerString(val);
						if (valLower == "driver")
						{
							out.asioConfig.bufferMode = RSAsioDevice::BufferSizeMode_Driver;
						}
						else if (valLower == "host")
						{
							out.asioConfig.bufferMode = RSAsioDevice::BufferSizeMode_Host;
						}
						else if (valLower == "custom")
						{
							out.asioConfig.bufferMode = RSAsioDevice::BufferSizeMode_Custom;
						}
						else
						{
							rslog::error_ts() << __FUNCTION__ << " - invalid value for buffer size mode. valid values are \"driver\", \"host\". line: " << line << std::endl;
						}
					}
					else if (key == "custombuffersize")
					{
						int s = 0;
						if (parseIntString(val, s) && s > 0)
						{
							out.asioConfig.customBufferSize = (unsigned)s;
						}
						else
						{
							rslog::error_ts() << __FUNCTION__ << " - invalid value for buffer size, value should be an integer starting at 1. line: " << line << std::endl;
						}
					}
				}
				else if (currentSection == SectionAsioOut)
				{
					if (key == "driver")
						out.asioConfig.output.asioDriverName = val;
					else if (key == "basechannel")
					{
						int c = 0;
						if (parseIntString(val, c) && c >= 0)
						{
							out.asioConfig.output.baseChannel = (unsigned)c;
						}
						else
						{
							rslog::error_ts() << __FUNCTION__ << " - invalid value for channel, value should be an integer starting at zero. line: " << line << std::endl;
						}
					}
					else if (key == "enablesoftwareendpointvolumecontrol")
					{
						parseBoolString(val, out.asioConfig.output.enableSoftwareEndpointVolumeControl);
					}
					else if (key == "enablesoftwaremastervolumecontrol")
					{
						parseBoolString(val, out.asioConfig.output.enableSoftwareMasterVolumeControl);
					}
					else if (key == "softwaremastervolumepercent")
					{
						int v = 0;
						if (parseIntString(val, v) && v >= 0)
						{
							out.asioConfig.output.softwareMasterVolumePercent = v;
						}
						else
						{
							rslog::error_ts() << __FUNCTION__ << " - invalid value for softwareMasterVolumePercent, value should be an integer between 0 and 1000. line: " << line << std::endl;
						}
					}
				}
				else if (currentSection == SectionAsioIn0 || currentSection == SectionAsioIn1 || currentSection == SectionAsioInMic)
				{
					RSAsioInputConfig& asioInputConfig = out.asioConfig.inputs[currentSection - SectionAsioIn0];

					if (currentSection == SectionAsioInMic)
						asioInputConfig.microphone = true;

					if (key == "driver")
						asioInputConfig.asioDriverName = val;
					else if (key == "channel")
					{
						int c = 0;
						if (parseIntString(val, c) && c >= 0)
						{
							asioInputConfig.useChannel = (unsigned)c;
						}
						else
						{
							rslog::error_ts() << __FUNCTION__ << " - invalid value for channel, value should be an integer starting at zero. line: " << line << std::endl;
						}
					}
					else if (key == "enablesoftwareendpointvolumecontrol")
					{
						parseBoolString(val, asioInputConfig.enableSoftwareEndpointVolumeControl);
					}
					else if (key == "enablesoftwaremastervolumecontrol")
					{
						parseBoolString(val, asioInputConfig.enableSoftwareMasterVolumeControl);
					}
					else if (key == "softwaremastervolumepercent")
					{
						int v = 0;
						if (parseIntString(val, v) && v >= 0)
						{
							asioInputConfig.softwareMasterVolumePercent = v;
						}
						else
						{
							rslog::error_ts() << __FUNCTION__ << " - invalid value for softwareMasterVolumePercent, value should be an integer between 0 and 1000. line: " << line << std::endl;
						}
					}
				}
			}
		}
	}
}