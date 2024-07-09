//
// Created by Ahemd Max on 7/8/2024.
//

#include "Config.h"
#include "stdafx.h"

#define INI_FILE "ASIODriverRefresher.ini"
#define UNSET std::numeric_limits<unsigned>::max()


static AsioConfig config;
static bool configLoaded = false;


void LoadConfigIni(AsioConfig& out)
{
    const std::string configPath =  fh::GetFilePath(INI_FILE);
    out.loaded = false;

    out.bufferSizes.fill(UNSET);

    std::ifstream file;
    file.open(configPath, std::ifstream::in);

    if (!file.is_open())
    {
        logging::info_ts() << "failed to open config file" << std::endl;
        return;
    }

    std::string currentLine;
    size_t line = 0;
    std::string currentSection;
    int bufferSizeIndex = 0;

    while (std::getline(file, currentLine))
    {
        ++line;
        currentLine = trimString(currentLine);

        if (currentLine.empty() || currentLine[0] == ';' || currentLine[0] == '#'){
            continue;
        }
        if (currentLine[0] == '[' && currentLine.back() == ']') {
            // Section line
            currentSection = currentLine.substr(1, currentLine.size() - 2);
        } else {
            // Key-value line
            size_t equalPos = currentLine.find('=');
            if (equalPos == std::string::npos) {
                continue;
            }

            std::string key = currentLine.substr(0, equalPos);
            std::string value = currentLine.substr(equalPos + 1);

            // Remove whitespace from both ends of key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (currentSection == "Buffer.size.0" || currentSection == "Buffer.size.1") {
                if (key == "CustomBufferSize") {
                    config.bufferSizes[bufferSizeIndex] = std::stoi(value);
                    bufferSizeIndex++;
                    out.loaded = true;
                }
            }
        }

    }

    file.close();

}

unsigned GetConfigBufferSize(unsigned current) {
    AsioConfig config = GetConfig();
    if(config.loaded) {
        for (unsigned int bufferSize : config.bufferSizes) {
            if(current != bufferSize) {
                return bufferSize;
            }
        }
    }
    return UNSET;
}

boolean IsSet(unsigned buffer) {
    return UNSET != buffer;
}



AsioConfig& GetConfig()
{
    if (!configLoaded)
    {
        configLoaded = true;
        LoadConfigIni(config);
    }

    return config;
}
