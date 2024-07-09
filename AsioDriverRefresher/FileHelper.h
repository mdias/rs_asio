#pragma once
namespace fh
{
    std::string GetExecutablePath();
    std::string GetFilePath(const std::string& filename);
    void replaceToFile(const std::string& filename, const std::string& content);
    void deleteFileIfExists(const std::string& filename);
    bool fileExists(const std::string& filename);
    std::string readFileToString(const std::string& filename);
    std::uintmax_t getFileSize(const std::string& filename);
}