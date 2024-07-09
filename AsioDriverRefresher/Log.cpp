#include "stdafx.h"
#include "Utils.h"

#define LOG_TO_CONSOLE 0
#define LOG_FILENAME "ASIODriverRefresher.log"

#if !LOG_TO_CONSOLE
static std::filebuf logFileBuffer;
#endif

namespace logging {
#if !LOG_TO_CONSOLE
    std::ostream info(&logFileBuffer);
    std::ostream error(&logFileBuffer);
#else
	std::ostream info(std::cout.rdbuf());
	std::ostream error(std::cerr.rdbuf());
#endif

    std::ostream &info_ts() {
        return info <<  GetTimestamp() << " [INFO]  ";
    }

    std::ostream &error_ts() {

        return error <<  GetTimestamp() << " [ERROR]  ";
    }


    void InitLog() {
#if !LOG_TO_CONSOLE

        std::string logFilePath = fh::GetFilePath(LOG_FILENAME);


        std::uintmax_t fileSize = fh::getFileSize(LOG_FILENAME);

        // Calculate 1 MB in bytes (1 MB = 1024 * 1024 bytes)
        constexpr std::uintmax_t ONE_MB = 1024 * 1024;


        if (fileSize > ONE_MB) {
            fh::deleteFileIfExists(LOG_FILENAME);
        }

        // Open log file in append mode
        logFileBuffer.open(logFilePath, std::ios_base::out | std::ios_base::app);
#else
		AllocConsole();
		AttachConsole(GetCurrentProcessId());

		FILE* fDummy = nullptr;
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);

		info.set_rdbuf(std::cout.rdbuf());
		error.set_rdbuf(std::cerr.rdbuf());
#endif
    }


    void CleanupLog() {
#if !LOG_TO_CONSOLE
        logFileBuffer.close();
#else
#endif
    }
}
