#include "stdafx.h"

#include <atomic>

#define LOG_TO_CONSOLE 0

static TimeStamp initTimeStamp;

namespace rslog
{
	// ---- message construction (runs on the logging thread) ----

	FixedStreamBuf::int_type FixedStreamBuf::overflow(int_type ch)
	{
		m_Truncated = true;
		return traits_type::not_eof(ch);
	}

	std::streamsize FixedStreamBuf::xsputn(const char* s, std::streamsize n)
	{
		const std::streamsize room = static_cast<std::streamsize>(epptr() - pptr());
		const std::streamsize toCopy = n < room ? n : room;
		if (toCopy > 0)
		{
			memcpy(pptr(), s, static_cast<size_t>(toCopy));
			pbump(static_cast<int>(toCopy));
		}
		if (toCopy < n)
			m_Truncated = true;
		return n;
	}

	namespace
	{
		// Bounded multi-producer / single-consumer FIFO over preallocated slots.
		// Statically zero-initialized (no dynamic init, usable before InitLog and
		// during static initialization). Each slot has a state that advances by 2
		// per lap around the ring: 2*lap = free for that lap, 2*lap+1 = full.
		constexpr std::size_t kRingSize = 4096; // power of two

		struct Slot
		{
			std::atomic<std::size_t> state;
			unsigned short length;
			char data[FixedStreamBuf::kCapacity];
		};

		struct Ring
		{
			std::atomic<std::size_t> enqueuePos;
			std::atomic<std::size_t> dequeuePos;
			Slot slots[kRingSize];
		};

		Ring g_ring;
		std::atomic<std::size_t> g_droppedMessages;

		// Called from any thread. Lock-free, never blocks, never allocates.
		void Enqueue(const char* data, std::size_t length, bool truncated)
		{
			std::size_t pos = g_ring.enqueuePos.load(std::memory_order_relaxed);
			for (;;)
			{
				Slot& slot = g_ring.slots[pos & (kRingSize - 1)];
				const std::size_t lap = pos / kRingSize;
				const std::size_t state = slot.state.load(std::memory_order_acquire);

				if (state == 2 * lap)
				{
					if (g_ring.enqueuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
					{
						if (length > sizeof(slot.data))
							length = sizeof(slot.data);
						memcpy(slot.data, data, length);
						if (truncated && length >= 4)
							memcpy(slot.data + length - 4, "...\n", 4);
						slot.length = static_cast<unsigned short>(length);
						slot.state.store(2 * lap + 1, std::memory_order_release);
						return;
					}
					// lost the race for this slot; pos was reloaded, retry
				}
				else if (state < 2 * lap)
				{
					// the slot hasn't been consumed since the previous lap: queue full
					g_droppedMessages.fetch_add(1, std::memory_order_relaxed);
					return;
				}
				else
				{
					pos = g_ring.enqueuePos.load(std::memory_order_relaxed);
				}
			}
		}

		// ---- consumer side: only ever used by one thread at a time ----

#if !LOG_TO_CONSOLE
		std::filebuf g_logFile;
#endif
		std::streambuf* g_sink = nullptr;

		// Writes up to a batch of queued messages to the sink; returns how many.
		std::size_t DrainOnce()
		{
			std::size_t count = 0;
			for (; count < 256; ++count)
			{
				const std::size_t pos = g_ring.dequeuePos.load(std::memory_order_relaxed);
				Slot& slot = g_ring.slots[pos & (kRingSize - 1)];
				const std::size_t lap = pos / kRingSize;
				if (slot.state.load(std::memory_order_acquire) != 2 * lap + 1)
					break;

				if (g_sink)
					g_sink->sputn(slot.data, slot.length);

				slot.state.store(2 * (lap + 1), std::memory_order_release);
				g_ring.dequeuePos.store(pos + 1, std::memory_order_relaxed);
			}

			const std::size_t dropped = g_droppedMessages.exchange(0, std::memory_order_relaxed);
			if (dropped && g_sink)
			{
				char note[96];
				const int len = snprintf(note, sizeof(note), "[ERROR]  %zu log message(s) dropped: log queue was full\n", dropped);
				if (len > 0)
					g_sink->sputn(note, len);
				++count;
			}

			if (count && g_sink)
				g_sink->pubsync();

			return count;
		}

		std::atomic<bool> g_stopRequested;
		HANDLE g_writerThread = nullptr;

		DWORD WINAPI WriterThreadProc(LPVOID)
		{
			for (;;)
			{
				if (DrainOnce() != 0)
					continue;

				if (g_stopRequested.load())
				{
					// one last pass in case something was queued right before the stop
					if (DrainOnce() == 0)
						break;
					continue;
				}

				Sleep(5);
			}
			return 0;
		}

		void AppendTimestamp(std::ostream& os)
		{
			// formatted by hand: no printf (its float path takes locks)
			const LONGLONG ms = (TimeStamp() - initTimeStamp).GetMicrosecs() / 1000;
			const LONGLONG secs = ms / 1000;
			const int frac = static_cast<int>(ms % 1000);
			os << secs << '.' << static_cast<char>('0' + frac / 100) << static_cast<char>('0' + (frac / 10) % 10) << static_cast<char>('0' + frac % 10);
		}
	}

	LogLine::LogLine(Kind kind)
		: m_Stream(&m_Buf)
	{
		if (kind != Kind::Raw)
		{
			AppendTimestamp(m_Stream);
			m_Stream << (kind == Kind::Info ? " [INFO]  " : " [ERROR]  ");
		}
	}

	LogLine::~LogLine()
	{
		Enqueue(m_Buf.data(), m_Buf.size(), m_Buf.truncated());
	}

	const LogSource info;
	const LogSource error;

	LogLine info_ts()
	{
		return LogLine(LogLine::Kind::Info);
	}

	LogLine error_ts()
	{
		return LogLine(LogLine::Kind::Error);
	}

	void InitLog()
	{
#if !LOG_TO_CONSOLE
		if (!g_logFile.open("RS_ASIO-log.txt", std::ios_base::out | std::ios_base::trunc))
			return;
		g_sink = &g_logFile;
#else
		AllocConsole();
		AttachConsole(GetCurrentProcessId());

		FILE* fDummy = nullptr;
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);

		g_sink = std::cout.rdbuf();
#endif
		initTimeStamp = TimeStamp();

		g_stopRequested = false;
		g_writerThread = CreateThread(nullptr, 0, WriterThreadProc, nullptr, 0, nullptr);
	}

	void CleanupLog()
	{
		g_stopRequested = true;

		bool consumerIsGone = true;
		if (g_writerThread)
		{
			// On process exit the writer has already been terminated by the OS and
			// this returns at once. On an explicit unload it finishes its final pass.
			consumerIsGone = (WaitForSingleObject(g_writerThread, 2000) == WAIT_OBJECT_0);
			CloseHandle(g_writerThread);
			g_writerThread = nullptr;
		}

		if (consumerIsGone)
		{
			// we are now the only consumer: write out whatever is still queued
			while (DrainOnce() != 0)
			{
			}

#if !LOG_TO_CONSOLE
			g_logFile.close();
#endif
			g_sink = nullptr;
		}
	}
}