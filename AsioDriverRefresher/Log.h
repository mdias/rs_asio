#pragma once
namespace logging
{
	extern std::ostream info;
	extern std::ostream error;

	std::ostream& info_ts();
	std::ostream& error_ts();

	void InitLog();
	void CleanupLog();
}