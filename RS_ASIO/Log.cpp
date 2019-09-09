#include "stdafx.h"

#define LOG_TO_CONSOLE 0

#if !LOG_TO_CONSOLE
static std::filebuf logFileBuffer;
#endif
static TimeStamp initTimeStamp;

namespace rslog
{
#if !LOG_TO_CONSOLE
	std::ostream info(&logFileBuffer);
	std::ostream error(&logFileBuffer);
#else
	std::ostream info(std::cout.rdbuf());
	std::ostream error(std::cerr.rdbuf());
#endif

	std::ostream& info_ts()
	{
		char tmp[16];
		snprintf(tmp, sizeof(tmp), "%0.3lf  ", (TimeStamp() - initTimeStamp).GetSeconds());

		return info << tmp;
	}

	std::ostream& error_ts()
	{
		char tmp[16];
		snprintf(tmp, sizeof(tmp), "%0.3lf  ", (TimeStamp() - initTimeStamp).GetSeconds());

		return error << tmp;
	}

	void InitLog()
	{
#if !LOG_TO_CONSOLE
		logFileBuffer.open("RS_ASIO-log.txt", std::ios_base::out | std::ios_base::trunc);
		initTimeStamp = TimeStamp();
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

	void CleanupLog()
	{
#if !LOG_TO_CONSOLE
		logFileBuffer.close();
#else
#endif
	}
}