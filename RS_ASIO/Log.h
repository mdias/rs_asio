#pragma once

#include <cstddef>
#include <ostream>
#include <streambuf>
#include <utility>

namespace rslog
{
	// Logging is safe to call from any thread, including threads the host
	// runtime doesn't know about (e.g. WineASIO invoking the ASIO callbacks from a
	// PipeWire thread, where making a Wine syscall crashes the process).
	//
	// A log statement formats into a fixed buffer on the caller's stack and, when
	// the statement ends, pushes the finished message into a lock-free bounded
	// FIFO. No heap allocation, no file I/O and no blocking lock happen on the
	// calling thread. A single dedicated writer thread, created at InitLog(),
	// drains the FIFO in order and writes to the log file. If the FIFO is full the
	// message is dropped and a notice with the drop count is logged later.

	// Fixed-capacity streambuf; output beyond the capacity is discarded.
	class FixedStreamBuf : public std::streambuf
	{
	public:
		static constexpr std::size_t kCapacity = 1024;

		FixedStreamBuf() { setp(m_Data, m_Data + kCapacity); }

		const char* data() const { return m_Data; }
		std::size_t size() const { return static_cast<std::size_t>(pptr() - pbase()); }
		bool truncated() const { return m_Truncated; }

	protected:
		int_type overflow(int_type ch) override;
		std::streamsize xsputn(const char* s, std::streamsize n) override;

	private:
		char m_Data[kCapacity];
		bool m_Truncated = false;
	};

	// One log message. Everything streamed into it becomes a single FIFO entry
	// that is enqueued when the object is destroyed, i.e. at the end of the
	// full-expression: rslog::info_ts() << "a" << 1 << std::endl;
	class LogLine
	{
	public:
		enum class Kind { Raw, Info, Error };

		explicit LogLine(Kind kind);

		template<typename T>
		LogLine(Kind kind, T&& first)
			: LogLine(kind)
		{
			m_Stream << std::forward<T>(first);
		}

		~LogLine();

		LogLine(const LogLine&) = delete;
		LogLine& operator=(const LogLine&) = delete;

		template<typename T>
		LogLine& operator<<(T&& value)
		{
			m_Stream << std::forward<T>(value);
			return *this;
		}

		LogLine& operator<<(std::ostream& (*manip)(std::ostream&))
		{
			manip(m_Stream);
			return *this;
		}

		LogLine& operator<<(std::ios_base& (*manip)(std::ios_base&))
		{
			manip(m_Stream);
			return *this;
		}

	private:
		FixedStreamBuf m_Buf;
		std::ostream m_Stream;
	};

	// Entry point for "rslog::info << ..." (no timestamp prefix).
	class LogSource
	{
	public:
		constexpr LogSource() = default;

		template<typename T>
		LogLine operator<<(T&& value) const
		{
			return LogLine(LogLine::Kind::Raw, std::forward<T>(value));
		}

		LogLine operator<<(std::ostream& (*manip)(std::ostream&)) const
		{
			return LogLine(LogLine::Kind::Raw, manip);
		}

		LogLine operator<<(std::ios_base& (*manip)(std::ios_base&)) const
		{
			return LogLine(LogLine::Kind::Raw, manip);
		}
	};

	extern const LogSource info;
	extern const LogSource error;

	// Timestamped, level-tagged messages.
	LogLine info_ts();
	LogLine error_ts();

	// Opens the log file and starts the writer thread.
	void InitLog();
	// Stops the writer thread and writes out anything still queued.
	void CleanupLog();
}