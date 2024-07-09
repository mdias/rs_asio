#include "stdafx.h"
#include <filesystem>

namespace fs = std::filesystem;
namespace fh
{

    std::string GetExecutablePath() {
        char buffer[MAX_PATH];
        GetModuleFileName(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        return std::string(buffer).substr(0, pos);
    }


    std::string GetFilePath(const std::string& filename) {
        return GetExecutablePath() + "\\" + filename;
    }

    std::uintmax_t getFileSize(const std::string& filename) {
        try {
            // Use std::filesystem::file_size to get the size of the file
            return fs::file_size(GetFilePath(filename));
        } catch (const std::filesystem::filesystem_error& ex) {
            return 0; // Return 0 or handle error as appropriate for your application
        }
    }


    void replaceToFile(const std::string& filename, const std::string& content) {
        // Open the file in output mode, which will truncate the file if it exists
        std::ofstream file(GetFilePath(filename), std::ios::out | std::ios::trunc);

        // Check if the file was opened successfully
        if (!file.is_open()) {
            logging::error_ts() << "Failed to open the file: " << filename << std::endl;
            return;
        }

        // Write the new content to the file
        file << content;

        // Close the file
        file.close();

        // Check for any errors during the write process
        if (file.fail()) {
            logging::error_ts() << "Error occurred while writing to the file: " << filename << std::endl;
        } else {
            logging::info_ts() << "File content replaced successfully." << std::endl;
        }
    }

    bool fileExists(const std::string& filename) {
        std::string path = GetFilePath(filename);
        std::ifstream file(path);
        return file.good();
    }

    // Function to delete a file if it exists
    void deleteFileIfExists(const std::string& filename) {
        if (fileExists(filename)) {
            std::string path = GetFilePath(filename);
            // Use std::remove to delete the file
            if (std::remove(path.c_str()) != 0) {
                logging::error_ts() << "Failed to delete the file: " << filename << std::endl;
            } else {
                logging::info_ts() << "File deleted successfully: " << filename << std::endl;
            }
        } else {
            logging::info_ts() << "File does not exist: " << filename << std::endl;
        }
    }

    std::string readFileToString(const std::string& filename) {
        std::ifstream file(GetFilePath(filename));
        std::stringstream buffer;

        if (file) {
            // Read file contents into the stringstream buffer
            buffer << file.rdbuf();
            file.close();

            // Return the content as a std::string
            return buffer.str();
        } else {
            // Handle error if file cannot be opened
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }


}