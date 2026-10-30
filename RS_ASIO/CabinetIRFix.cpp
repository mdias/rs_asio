#include "stdafx.h"
#include "CabinetIRFix.h"
#include "Patcher.h"
#include "Configurator.h"
#include <windows.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include <mutex>
#include <map>
#include <complex>
#include <cmath>

namespace
{
	struct ModuleImage
	{
		const BYTE* base = nullptr;
		size_t size = 0;
	};

	ModuleImage GetGameImage()
	{
		ModuleImage img;
		HMODULE game = GetModuleHandleA(nullptr);
		if (!game)
			return img;

		const BYTE* base = reinterpret_cast<const BYTE*>(game);
		const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
		if (dos->e_magic != IMAGE_DOS_SIGNATURE)
			return img;

		const IMAGE_NT_HEADERS32* nt =
			reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE)
			return img;

		img.base = base;
		img.size = nt->OptionalHeader.SizeOfImage;
		return img;
	}

	// The executable is packed, so section characteristics at rest do not
	// describe what is mapped: a packed image can leave ranges reserved but not
	// committed. Every read is therefore guarded.
	bool SafeRead(const void* p, void* out, size_t n)
	{
		if (!p)
			return false;
		__try
		{
			memcpy(out, p, n);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Guarded write counterpart to SafeRead. The media buffer is bank-owned;
	// a partial or faulting write must not take the audio/loading thread down.
	bool SafeWrite(void* p, const void* in, size_t n)
	{
		if (!p)
			return false;
		__try
		{
			memcpy(p, in, n);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Converted-payload cache.
	//
	// A cabinet swap in Tone Designer re-fetches the media, and the bank hands
	// back a fresh copy still stamped at the authored rate - so without this the
	// whole decode/resample/encode runs again every single time, measured at
	// 10-19 ms per load. The game fades down across a cabinet change, which is
	// why this is survivable at all, but that work lands inside the fade and is
	// audible as a click at its edge. Cycling back to a cabinet already used in
	// the session is then free: a memcpy instead of partCount forward and
	// inverse transforms.
	//
	// Keyed on a 64-bit hash of the ENTIRE pre-conversion media plus the target
	// rate. Hashing ~40 KB costs microseconds against the milliseconds it saves,
	// and hashing the whole media (not a prefix) keeps cabinets that share their
	// first kilobyte apart. Equal hashes are not a proof of equal media.
	std::mutex g_CabinetCacheMutex;
	std::map<uint64_t, std::vector<unsigned char>> g_CabinetCache;
	constexpr size_t kCabinetCacheMaxEntries = 128;

	uint64_t CabinetCacheKey(const unsigned char* media, size_t size, unsigned rate)
	{
		uint64_t h = 1469598103934665603ull;               // FNV-1a 64
		for (size_t i = 0; i < size; ++i)
		{
			h ^= media[i];
			h *= 1099511628211ull;
		}
		h ^= static_cast<uint64_t>(rate);
		h *= 1099511628211ull;
		return h;
	}

	// -----------------------------------------------------------------------
	// Cabinet impulse-response resampler
	//
	// Decode the
	// IR's partitioned-convolution media to a time-domain waveform, resample
	// that waveform to the engine's rate, re-encode it into the same
	// fixed-size media layout, and rewrite the header's rate field so the
	// game's own mismatch check passes honestly instead of being disabled.
	//
	// Partitioned-convolution media format:
	//   +0x00  48-byte header, 12 dwords: [0]=block size(128) [1]=FFT size(256)
	//          [2]=sample rate [3]=channels(1) [4]=partition count [5..6]=gain,
	//          gainDb (float) [7]=partition count again [8]=IR length in
	//          samples [9]=block size again [10..11]=0
	//   +0x30  a uint16 table with hdr[4] entries, each the BIN COUNT of that
	//          partition (sum(table) == hdr[8]); usually all equal the block size,
	//          zero-padded up to a 16-byte boundary. NOT the fixed 16 floats of
	//          padding an earlier reading of this format assumed - it grows with
	//          the partition count, so the payload offset is variable:
	//              payload = 0x30 + ((2*partCount + 15) & ~0xF)
	//          Verified against all 440 shipped IRs and against the engine's own
	//          computation at RVA 0x00A84AF5.
	//   +off   hdr[4] partitions, each hdr[1] float32 = hdr[1]/2 complex bins
	//          interleaved [re0,im0,re1,im1,...], a 256-point REAL FFT stored
	//          as 128 bins. Wwise DISCARDS the Nyquist bin (129th); bin 0's
	//          imaginary part is literal zero in the data, NOT packed
	//          Nyquist. Decode must force Nyquist to 0; encode must write
	//          zero to bin 0's imaginary part. Getting this backwards doubles
	//          the reconstruction error (validated offline).
	//
	// Partition k is the transform of a 2*block window holding TWO consecutive
	// blocks: [block k-1, block k]. Partition 0's first half is silence (measured
	// at -73 to -105 dB across the corpus), and partition k's first half equals
	// the last block[k] samples of partition k-1's second half - verified on all
	// 440 shipped IRs, tapered tables included. So:
	//
	//   decode: partition k contributes time[block .. 2*block), placed at
	//           sum(table[0..k]). Concatenating SECOND halves gives the IR.
	//   encode: partition k = [ ir[off-block .. off), ir[off .. off+block) ],
	//           with the first half zero for k = 0.
	//
	// Taking first halves instead reconstructs the IR delayed by one block and
	// drops the final block. That delay is invisible in a round trip - encode
	// reintroduces it - but it does NOT survive a rate change: 128 samples in
	// becomes 128*ratio samples out, so ~10 samples of the IR's leading transient
	// are cut off at 44.1 kHz. Measured up to 13.5 dB of magnitude-response error,
	// worst on IRs with a hard normalised onset, where it is audible as a
	// gritty top end.

	// In-place iterative radix-2 Cooley-Tukey, with a direct-DFT fallback for
	// the non-power-of-two sizes that variable partitions need.
	void ComplexFFT(std::complex<double>* a, size_t n, bool inverse)
	{
		// Variable-partition media needs non-power-of-two transforms (a 58-bin
		// partition is a 116-point FFT). The radix-2 path below assumes a power
		// of two - its bit-reversal silently produces garbage otherwise - so
		// those sizes take a direct DFT. n <= 256 here, so O(n^2) is a few tens
		// of thousands of operations at cabinet load time, not in the callback.
		if (n > 1 && (n & (n - 1)) != 0)
		{
			std::vector<std::complex<double>> out(n);
			const double sign = inverse ? 1.0 : -1.0;
			for (size_t k = 0; k < n; ++k)
			{
				std::complex<double> acc(0.0, 0.0);
				for (size_t t = 0; t < n; ++t)
				{
					const double ang = sign * 2.0 * 3.14159265358979323846
						* static_cast<double>(k) * static_cast<double>(t) / static_cast<double>(n);
					acc += a[t] * std::complex<double>(std::cos(ang), std::sin(ang));
				}
				// Same 1/n inverse scaling as the radix-2 path below.
				out[k] = inverse ? acc / static_cast<double>(n) : acc;
			}
			for (size_t i = 0; i < n; ++i)
				a[i] = out[i];
			return;
		}
		for (size_t i = 1, j = 0; i < n; ++i)
		{
			size_t bit = n >> 1;
			for (; j & bit; bit >>= 1)
				j ^= bit;
			j ^= bit;
			if (i < j)
				std::swap(a[i], a[j]);
		}
		for (size_t len = 2; len <= n; len <<= 1)
		{
			const double ang = 2.0 * 3.14159265358979323846 / static_cast<double>(len)
				* (inverse ? 1.0 : -1.0);
			const std::complex<double> wlen(std::cos(ang), std::sin(ang));
			for (size_t i = 0; i < n; i += len)
			{
				std::complex<double> w(1.0, 0.0);
				for (size_t j = 0; j < len / 2; ++j)
				{
					const std::complex<double> u = a[i + j];
					const std::complex<double> v = a[i + j + len / 2] * w;
					a[i + j] = u + v;
					a[i + j + len / 2] = u - v;
					w *= wlen;
				}
			}
		}
		if (inverse)
		{
			for (size_t i = 0; i < n; ++i)
				a[i] /= static_cast<double>(n);
		}
	}

	// Forward real FFT: n real samples -> n/2+1 complex bins (0..Nyquist).
	std::vector<std::complex<double>> RealForwardFFT(const double* x, size_t n)
	{
		std::vector<std::complex<double>> a(n);
		for (size_t i = 0; i < n; ++i)
			a[i] = std::complex<double>(x[i], 0.0);
		ComplexFFT(a.data(), n, false);
		return std::vector<std::complex<double>>(a.begin(), a.begin() + (n / 2 + 1));
	}

	// Inverse real FFT: n/2+1 complex bins (conjugate-symmetric spectrum
	// assumed) -> n real samples.
	std::vector<double> RealInverseFFT(const std::complex<double>* bins, size_t n)
	{
		std::vector<std::complex<double>> a(n);
		a[0] = std::complex<double>(bins[0].real(), 0.0);
		for (size_t i = 1; i < n / 2; ++i)
		{
			a[i] = bins[i];
			a[n - i] = std::conj(bins[i]);
		}
		a[n / 2] = std::complex<double>(bins[n / 2].real(), 0.0);
		ComplexFFT(a.data(), n, true);
		std::vector<double> out(n);
		for (size_t i = 0; i < n; ++i)
			out[i] = a[i].real();
		return out;
	}

	// Windowed-sinc resampler (Blackman window, >=16 taps/output sample).
	//
	// The kernel depends only on the FRACTIONAL part of the source position, and
	// for integer rates that fraction takes exactly L distinct values, where
	// L = rateOut / gcd(rateIn, rateOut). 48000 -> 44100 is 147/160, so there are
	// 147 phases, not one per output sample. Building the kernel once per phase
	// turns the inner loop into pure multiply-add.
	//
	// This matters because it is not a micro-optimisation: the direct form calls
	// sin() and cos() twice per tap, ~343k times for one cabinet, and measured
	// 9-21 ms per conversion. The game fades down across a cabinet swap, so that
	// cost lands inside the fade and is audible as a click at its edge.
	// Same kernel, same arithmetic - validated to 4.4e-13 against the direct form
	// with a phase-shifted control that fails at 8e-1, so the check discriminates.
	std::vector<double> WindowedSincResample(const std::vector<double>& x,
		double rateIn, double rateOut)
	{
		if (rateIn <= 0.0 || rateOut <= 0.0 || rateIn == rateOut)
			return x;

		const double ratio = rateOut / rateIn;
		const size_t nIn = x.size();
		const size_t nOut = static_cast<size_t>(
			std::llround(static_cast<double>(nIn) * ratio));
		if (nOut == 0 || nIn == 0)
			return {};

		// cutoff tracks the lower of the two Nyquist rates, so the same
		// kernel also serves as the anti-alias filter when downsampling.
		const double cutoff = (std::min)(1.0, ratio);
		constexpr double kTapsPerOutput = 32.0;  // well above the 16-tap floor
		const double halfWidth = kTapsPerOutput / (std::max)(cutoff, 1e-6);

		// Rational phase count. Non-integer or awkward rate pairs (a huge L means
		// the table would cost more than it saves) fall through to the direct form,
		// which stays correct - just slower.
		const long long rIn = std::llround(rateIn);
		const long long rOut = std::llround(rateOut);
		long long L = 0, M = 0;
		if (rIn > 0 && rOut > 0
			&& std::fabs(rateIn - static_cast<double>(rIn)) < 1e-6
			&& std::fabs(rateOut - static_cast<double>(rOut)) < 1e-6)
		{
			long long a = rIn, b = rOut;
			while (b != 0) { const long long t = a % b; a = b; b = t; }
			if (a > 0)
			{
				L = rOut / a;
				M = rIn / a;
			}
		}
		constexpr long long kMaxPhases = 8192;

		std::vector<double> y(nOut, 0.0);

		if (L > 0 && L <= kMaxPhases)
		{
			const long jLo = static_cast<long>(std::floor(0.0 - halfWidth)) - 1;
			const long jHi = static_cast<long>(std::ceil(1.0 + halfWidth)) + 1;
			const size_t span = static_cast<size_t>(jHi - jLo + 1);

			std::vector<double> kernel(static_cast<size_t>(L) * span, 0.0);
			for (long long q = 0; q < L; ++q)
			{
				const double f = static_cast<double>(q) / static_cast<double>(L);
				double* row = kernel.data() + static_cast<size_t>(q) * span;
				for (long j = jLo; j <= jHi; ++j)
				{
					const double d = static_cast<double>(j) - f;
					const double t = d * cutoff;
					const double sinc = (std::fabs(t) < 1e-9)
						? 1.0 : std::sin(3.14159265358979323846 * t) / (3.14159265358979323846 * t);
					double u = d / halfWidth;
					u = (std::max)(-1.0, (std::min)(1.0, u));
					const double window = 0.42 + 0.5 * std::cos(3.14159265358979323846 * u)
						+ 0.08 * std::cos(2.0 * 3.14159265358979323846 * u);
					row[static_cast<size_t>(j - jLo)] = sinc * cutoff * window;
				}
			}

			for (size_t m = 0; m < nOut; ++m)
			{
				const long long mm = static_cast<long long>(m) * M;
				const long long base = mm / L;
				const double* row = kernel.data()
					+ static_cast<size_t>(mm % L) * span;

				long long n0 = base + jLo;
				long long n1 = base + jHi;
				if (n0 < 0) n0 = 0;
				if (n1 > static_cast<long long>(nIn) - 1)
					n1 = static_cast<long long>(nIn) - 1;

				double sum = 0.0;
				for (long long n = n0; n <= n1; ++n)
					sum += x[static_cast<size_t>(n)]
						* row[static_cast<size_t>(n - base - jLo)];
				y[m] = sum;
			}
			return y;
		}

		// Direct form - correct for any rate pair, used when the phase table
		// would not pay for itself.
		for (size_t m = 0; m < nOut; ++m)
		{
			const double center = static_cast<double>(m) / ratio;
			long lo = static_cast<long>(std::floor(center - halfWidth));
			long hi = static_cast<long>(std::ceil(center + halfWidth));
			if (lo < 0) lo = 0;
			if (hi > static_cast<long>(nIn) - 1) hi = static_cast<long>(nIn) - 1;
			if (lo > hi)
				continue;

			double sum = 0.0;
			for (long n = lo; n <= hi; ++n)
			{
				const double t = (static_cast<double>(n) - center) * cutoff;
				const double sinc = (std::fabs(t) < 1e-9)
					? 1.0 : std::sin(3.14159265358979323846 * t) / (3.14159265358979323846 * t);
				double u = (static_cast<double>(n) - center) / halfWidth;
				u = (std::max)(-1.0, (std::min)(1.0, u));
				const double window = 0.42 + 0.5 * std::cos(3.14159265358979323846 * u)
					+ 0.08 * std::cos(2.0 * 3.14159265358979323846 * u);
				sum += x[static_cast<size_t>(n)] * sinc * cutoff * window;
			}
			y[m] = sum;
		}
		return y;
	}

	// Takes the media the ENGINE already fetched, and the engine rate by value.
	//
	// It must NOT call GetPluginMedia itself: a second fetch acquires the media
	// twice while the engine releases it once, which corrupts the heap. Exactly
	// one call per Init is what the engine expects, so we read the pointer it
	// already holds.
	void ResampleCabinetIR(unsigned char* data, unsigned size, unsigned engineRate)
	{
		// Wall-clock cost of the whole hook. The game fades down across a
		// cabinet swap, so what matters is whether this fits inside that fade;
		// measure the conversion directly rather than inferring it from logs.
		LARGE_INTEGER perfFreq, perfStart;
		QueryPerformanceFrequency(&perfFreq);
		QueryPerformanceCounter(&perfStart);
		double hashMs = 0.0;
		if (!data || size < 0x80 || engineRate == 0)
			return;

		unsigned hdr[12] = {};
		if (!SafeRead(data, hdr, sizeof(hdr)))
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: media header unreadable" << std::endl;
			return;
		}

		const unsigned blockSize = hdr[0];
		const unsigned fftSize = hdr[1];
		const unsigned mediaRate = hdr[2];
		const unsigned partCount = hdr[4];
		const unsigned irLength = hdr[8];

		// Fail closed on anything that doesn't look like the confirmed
		// format - a guess here would corrupt bank-owned memory.

		// The FFT payload does NOT start at a fixed offset. Between the 48-byte
		// header and the payload sits a partCount-entry table of uint16 block
		// sizes, zero-padded up to a 16-byte boundary. The engine computes this
		// itself at RVA 0x00A84AF5:
		//     lea edx,[ecx+ecx+0xf] ; and edx,0xfffffff0   with ecx = partCount
		// It equals 0x70 only for 25..32 partitions; a hardcoded 0x70 writes
		// past the media block on larger cabinets and corrupts the heap. 331 of
		// the 440 shipped IRs have 33+ partitions.
		// Bound everything the header claims against the media size before it
		// sizes any allocation or read: a malformed header must not be able to
		// request a huge table or read past the media.
		const size_t maxPartCount = (size - 0x30) / sizeof(unsigned short);
		if (partCount == 0 || partCount > maxPartCount || mediaRate < 8000 || mediaRate > 192000)
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: header out of range (partCount="
				<< partCount << " mediaRate=" << mediaRate << " size=" << size
				<< "); not touching media" << std::endl;
			return;
		}
		const size_t payloadOffset = 0x30 + ((2u * static_cast<size_t>(partCount) + 15u) & ~static_cast<size_t>(0xF));
		if (payloadOffset > size)
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: partition table does not fit the media; "
				"not touching media" << std::endl;
			return;
		}
		// Channel count. Multi-channel media stores one COMPLETE payload block
		// per channel, laid end to end - the engine points channel 2 at
		// payload + irLength*8 (== the whole partition payload) in the
		// branch at 0x00A84B01. Not interleaved, and not packed into the table.
		//
		// Every one of the 440 shipped cabinets is mono, so this path has never
		// run on real content. Converting only
		// channel 1 would leave channel 2 authored at the old rate, which sounds
		// like the stereo image collapsing rather than an obvious fault - worse
		// than refusing to touch it at all.
		const unsigned channels = hdr[3];
		if (channels == 0 || channels > 8)
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: implausible channel count "
				<< channels << "; not touching media" << std::endl;
			return;
		}
		// The uint16 table at +0x30 is the PER-PARTITION BIN COUNT, not a
		// constant block size. Partition k holds table[k] complex bins - two
		// floats each, so 8*table[k] bytes - and sum(table) == irLength.
		// Verified across all 444 media in the shipped banks, 0 violations, and
		// confirmed independently: the size this rule predicts for cab_pa600c
		// (10512 bytes) is exactly the size the running game reported for it.
		//
		// 440 of the 444 are uniform (every entry 128), which is why a fixed
		// stride worked at all. The other four taper their tails - cab_gb412cmki
		// ends 128, 58, 58, 30, 28, 10, 8 - and used to be refused outright,
		// costing those cabinets their speaker at any non-authored rate.
		std::vector<unsigned short> partBins(partCount ? partCount : 1, 0);
		if (partCount == 0 || !SafeRead(data + 0x30, partBins.data(),
				static_cast<size_t>(partCount) * sizeof(unsigned short)))
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: partition table unreadable "
				"(partCount=" << partCount << "); not touching media" << std::endl;
			return;
		}
		// Byte offset of each partition within a channel, and the sample index
		// it starts at. Both are prefix sums over the table.
		std::vector<size_t> partByteOff(partCount + 1, 0);
		std::vector<size_t> partSampleOff(partCount + 1, 0);
		size_t binTotal = 0;
		for (unsigned k = 0; k < partCount; ++k)
		{
			// The transform supports at most 128 bins (a 256-point FFT).
			if (partBins[k] == 0 || partBins[k] > 128)
			{
				rslog::error_ts() << "CABINET_IR_RESAMPLE: partition " << k << " has "
					<< partBins[k] << " bins (supported 1..128); not touching media" << std::endl;
				return;
			}
			partByteOff[k] = binTotal * 8;
			partSampleOff[k] = binTotal;
			binTotal += partBins[k];
		}
		partByteOff[partCount] = binTotal * 8;
		partSampleOff[partCount] = binTotal;

		// 64-bit so the total cannot wrap on x86.
		const size_t channelBytes = binTotal * 8;
		const uint64_t requiredSize = static_cast<uint64_t>(payloadOffset)
			+ static_cast<uint64_t>(channelBytes) * channels;
		if (blockSize == 0 || fftSize == 0 || partCount == 0 || irLength == 0
			|| binTotal != irLength
			|| static_cast<uint64_t>(size) < requiredSize)
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: header does not look sane "
				"(blockSize=" << blockSize << " fftSize=" << fftSize
				<< " partCount=" << partCount << " irLength=" << irLength
				<< " sum(table)=" << binTotal
				<< " size=" << size << " required=" << requiredSize
				<< " payloadOffset=0x" << std::hex << payloadOffset << std::dec
				<< "); not touching media" << std::endl;
			return;
		}

		if (mediaRate == engineRate)
			return;  // already matches; idempotent by construction

		// One complete payload block per channel, converted independently.
		// Nothing is written until every channel has encoded cleanly, so a
		// failure partway through leaves the media untouched rather than
		// half-converted at two different rates.
		// Cache lookup. A hit means this exact media has already been converted
		// to this exact rate in this session, so the filter is byte-identical and
		// there is nothing to recompute.
		std::vector<unsigned char> mediaCopy(size);
		uint64_t cacheKey = 0;
		bool haveCacheKey = false;
		LARGE_INTEGER hashStart, hashEnd;
		QueryPerformanceCounter(&hashStart);
		if (SafeRead(data, mediaCopy.data(), size))
		{
			cacheKey = CabinetCacheKey(mediaCopy.data(), size, engineRate);
			haveCacheKey = true;
		}
		QueryPerformanceCounter(&hashEnd);
		hashMs = 1000.0 * double(hashEnd.QuadPart - hashStart.QuadPart)
			/ double(perfFreq.QuadPart);

		std::vector<std::vector<unsigned char>> encodedPerChannel;
		bool fromCache = false;
		if (haveCacheKey)
		{
			std::lock_guard<std::mutex> lock(g_CabinetCacheMutex);
			auto it = g_CabinetCache.find(cacheKey);
			if (it != g_CabinetCache.end()
				&& it->second.size() == channelBytes * static_cast<size_t>(channels))
			{
				encodedPerChannel.reserve(channels);
				for (unsigned ch = 0; ch < channels; ++ch)
				{
					const unsigned char* src = it->second.data()
						+ static_cast<size_t>(ch) * channelBytes;
					encodedPerChannel.emplace_back(src, src + channelBytes);
				}
				fromCache = true;
			}
		}

		if (!fromCache)
		{
		encodedPerChannel.reserve(channels);
		for (unsigned ch = 0; ch < channels; ++ch)
		{
			const size_t chBase = payloadOffset + static_cast<size_t>(ch) * channelBytes;

			// --- decode: partitions -> time-domain IR ---
			std::vector<double> ir(irLength, 0.0);
			{
				std::vector<float> raw;
				for (unsigned k = 0; k < partCount; ++k)
				{
					const size_t kBins = partBins[k];
					if (kBins == 0)
						continue;               // pa999c's final entry is a legitimate 0
					const size_t kFft = kBins * 2;
					raw.resize(kBins * 2);
					if (!SafeRead(data + chBase + partByteOff[k], raw.data(), kBins * 8))
					{
						rslog::error_ts() << "CABINET_IR_RESAMPLE: partition " << k
							<< " unreadable; aborting" << std::endl;
						return;
					}
					std::vector<std::complex<double>> bins(kBins + 1);
					for (size_t b = 0; b < kBins; ++b)
						bins[b] = std::complex<double>(raw[b * 2 + 0], raw[b * 2 + 1]);
					bins[kBins] = std::complex<double>(0.0, 0.0);  // Nyquist: discarded, not packed

					// The frame holds TWO blocks: [previous block, this block]. This
					// block - the partition's actual contribution to the IR - is the
					// SECOND half. Reading the first half instead yields the whole IR
					// delayed by one block and silently drops the final block.
					const std::vector<double> time = RealInverseFFT(bins.data(), kFft);
					for (size_t s = 0; s < kBins; ++s)
						ir[partSampleOff[k] + s] = time[kBins + s];
				}
			}

			// --- resample the waveform to the engine's rate ---
			std::vector<double> newIr = WindowedSincResample(ir,
				static_cast<double>(mediaRate), static_cast<double>(engineRate));

			// --- length handling: media size is bank-owned and fixed ---
			if (newIr.size() < irLength)
			{
				newIr.resize(irLength, 0.0);  // e.g. 44.1 kHz: shorter IR, zero-pad
			}
			else if (newIr.size() > irLength)
			{
				// e.g. 96 kHz: does not fit. Truncate and fade the tail so the cut
				// isn't a discontinuity. The IR's energy is overwhelmingly in the
				// first few partitions, so this costs tail, not character.
				//
				// InstallCabinetIRProbe() below now refuses to install this hook
				// except when GameSampleRate == 44100, so for 48 kHz media this
				// branch is unreachable; kept as a defensive fallback in case that
				// gate is ever bypassed (e.g. a future caller with a different
				// rate policy), so a mistake there still fails safely instead of
				// overrunning the media block.
				const size_t originalNewLen = newIr.size();
				newIr.resize(irLength);
				const size_t fadeLen = (std::min<size_t>)(64, irLength);
				for (size_t i = 0; i < fadeLen; ++i)
				{
					const double u = static_cast<double>(i) / static_cast<double>(fadeLen - 1);
					const double gain = 0.5 * (1.0 + std::cos(u * 3.14159265358979323846));
					newIr[irLength - fadeLen + i] *= gain;
				}
				rslog::info_ts() << "CABINET_IR_RESAMPLE: resampled IR (" << originalNewLen
					<< " samples) exceeds the fixed media length (" << irLength
					<< ") at engineRate=" << engineRate << "; truncated with a "
					<< fadeLen << "-sample raised-cosine fade" << std::endl;
			}

			// --- re-encode: time-domain IR -> partitions ---
			std::vector<unsigned char> encoded(channelBytes, 0);
			{
				std::vector<double> seg;
				std::vector<float> outRaw;
				for (unsigned k = 0; k < partCount; ++k)
				{
					const size_t kBins = partBins[k];
					if (kBins == 0)
						continue;
					const size_t kFft = kBins * 2;
					seg.assign(kFft, 0.0);
					const size_t base = partSampleOff[k];
					// Rebuild the [previous block, this block] frame the format wants.
					// Filling the frame with one contiguous 2*kBins run instead only
					// lines up when the decode is also off by a block, and those two
					// errors do NOT cancel across a rate change: the block delay is
					// kBins samples going in but kBins*ratio coming out, and the
					// difference is sliced off the IR's leading transient.
					for (size_t s = 0; s < kBins; ++s)
					{
						if (base >= kBins)
							seg[s] = newIr[base - kBins + s];
						if (base + s < irLength)
							seg[kBins + s] = newIr[base + s];
					}

					const std::vector<std::complex<double>> spec = RealForwardFFT(seg.data(), kFft);
					outRaw.resize(kBins * 2);
					for (size_t b = 0; b < kBins; ++b)
					{
						outRaw[b * 2 + 0] = static_cast<float>(spec[b].real());
						outRaw[b * 2 + 1] = static_cast<float>(spec[b].imag());
					}
					outRaw[1] = 0.0f;  // bin 0 imaginary part is literal zero in this format

					memcpy(encoded.data() + partByteOff[k], outRaw.data(), kBins * 8);
				}
			}
			encodedPerChannel.push_back(std::move(encoded));
		}
		}

		// Populate the cache from whatever we just computed, so the next swap
		// back to this cabinet is a memcpy. Bounded so a long session cycling
		// every cabinet cannot grow without limit; cabinets are ~26-53 KB each.
		if (!fromCache && haveCacheKey)
		{
			std::vector<unsigned char> flat;
			flat.reserve(channelBytes * static_cast<size_t>(channels));
			for (unsigned ch = 0; ch < channels; ++ch)
				flat.insert(flat.end(), encodedPerChannel[ch].begin(), encodedPerChannel[ch].end());
			std::lock_guard<std::mutex> lock(g_CabinetCacheMutex);
			if (g_CabinetCache.size() >= kCabinetCacheMaxEntries)
				g_CabinetCache.clear();
			g_CabinetCache.emplace(cacheKey, std::move(flat));
		}

		// --- commit: rewrite header rate + partitions in the bank-owned buffer ---
		DWORD oldProtect = 0;
		if (!RedirectedVirtualProtect(data, size, PAGE_READWRITE, &oldProtect))
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: VirtualProtect(RW) failed; "
				"not writing" << std::endl;
			return;
		}

		bool ok = SafeWrite(data + 2 * 4, &engineRate, sizeof(engineRate));
		for (unsigned ch = 0; ok && ch < channels; ++ch)
		{
			const std::vector<unsigned char>& enc = encodedPerChannel[ch];
			const size_t chBase = payloadOffset + static_cast<size_t>(ch) * channelBytes;
			for (unsigned k = 0; ok && k < partCount; ++k)
			{
				const size_t kBytes = static_cast<size_t>(partBins[k]) * 8;
				if (kBytes == 0)
					continue;
				ok = SafeWrite(data + chBase + partByteOff[k],
					enc.data() + partByteOff[k], kBytes);
			}
		}

		DWORD restoreIgnored = 0;
		RedirectedVirtualProtect(data, size, oldProtect, &restoreIgnored);

		LARGE_INTEGER perfEnd;
		QueryPerformanceCounter(&perfEnd);
		const double elapsedMs = 1000.0 * double(perfEnd.QuadPart - perfStart.QuadPart)
			/ double(perfFreq.QuadPart);

		if (ok)
		{
			rslog::info_ts() << "CABINET_IR_RESAMPLE: resampled cabinet IR "
				<< mediaRate << " -> " << engineRate << " Hz (" << partCount
				<< " partitions, " << irLength << " samples, " << channels
				<< "ch, payload at 0x"
				<< std::hex << payloadOffset << std::dec << "); header rewritten, "
				"the engine's own rate check now passes honestly"
				<< (fromCache ? " [cached: no recompute]" : " [computed]")
				<< " took " << elapsedMs << " ms (key/copy " << hashMs << " ms)" << std::endl;
		}
		else
		{
			rslog::error_ts() << "CABINET_IR_RESAMPLE: write to media failed partway "
				"through - cabinet IR may be left inconsistent" << std::endl;
		}
	}

	void CabinetMediaHook(unsigned char* data, unsigned size, unsigned engineRate)
	{
		// Still runs before the engine acts on the header: the stolen
		// instructions we return to are only the null-check, and the rate
		// comparison that decides accept/reject is further down.
		ResampleCabinetIR(data, size, engineRate);
	}

	// Where execution resumes: the instruction after the five bytes we steal.
	static void* g_CabinetHookResume;

	// Hook sited AFTER the engine's own GetPluginMedia call, not at function
	// entry, so that call happens exactly once per Init. Calling it ourselves a
	// second time corrupts the heap (see ResampleCabinetIR).
	//
	//   00A84AA2  ff d0     call eax          ; the engine's single fetch
	//   00A84AA4  8b 45 fc  mov eax,[ebp-4]   ; 3 bytes  <-- stolen
	//   00A84AA7  85 c0     test eax,eax      ; 2 bytes  <-- stolen  (5 total)
	//   00A84AA9  75 0c     jne ...           ; CONSUMES the flags
	//
	// ebp still addresses Init's frame here, so [ebp-4] is the media pointer,
	// [ebp-8] its size and [ebp+0x10] a pointer to the engine rate.
	//
	// The stolen `test` is re-executed LAST, immediately before returning, so the
	// flags the following jne depends on are the ones it expects. Doing our work
	// after the test would clobber them.
	__declspec(naked) void CabinetMediaThunk()
	{
		__asm
		{
			pushfd
			pushad
			mov  eax, [ebp + 0x10]      // pointer to the engine rate
			test eax, eax
			jz   rsasio_cab_skip
			mov  eax, [eax]             // engine rate by value
			push eax
			mov  ecx, [ebp - 8]         // media size
			push ecx
			mov  edx, [ebp - 4]         // media pointer, already fetched
			push edx
			call CabinetMediaHook
			add  esp, 12
		rsasio_cab_skip:
			popad
			popfd
			mov  eax, [ebp - 4]         // stolen instruction 1
			test eax, eax               // stolen instruction 2 - sets the flags
			jmp  dword ptr [g_CabinetHookResume]
		}
	}

	bool InstallCabinetIRProbeAt(const ModuleImage& img, uintptr_t siteRva)
	{
		// Init entry/site RVAs: 2022 0x00A84A80/0x00A84AA4;
		// LP December 2024 0x00A83A80/0x00A83AA4. Both steal the same bytes.
		// NOT the function entry. This is the instruction right after Init's own
		// call to GetPluginMedia, so [ebp-4]/[ebp-8] already hold the media the
		// engine fetched. Hooking the entry and fetching our own copy corrupts
		// the heap.
		static const BYTE expected[]{ 0x8B, 0x45, 0xFC, 0x85, 0xC0 };  // mov eax,[ebp-4] / test eax,eax
		constexpr size_t kStolen = sizeof(expected);

		if (!img.base || siteRva + kStolen >= img.size)
			return false;

		BYTE* const target = const_cast<BYTE*>(img.base) + siteRva;
		BYTE actual[kStolen] = {};
		if (!SafeRead(target, actual, kStolen) || memcmp(actual, expected, kStolen) != 0)
		{
			rslog::error_ts() << "CABINET_IR: byte mismatch at RVA 0x" << std::hex
				<< siteRva << std::dec << "; not hooking" << std::endl;
			return false;
		}

		// The thunk re-executes both stolen instructions, so it resumes at the
		// jne that consumes the flags test sets.
		g_CabinetHookResume = target + kStolen;

		BYTE patch[kStolen];
		memset(patch, 0x90, sizeof(patch));
		patch[0] = 0xE9;
		const intptr_t fwd = reinterpret_cast<BYTE*>(&CabinetMediaThunk) - (target + 5);
		memcpy(patch + 1, &fwd, 4);

		if (!Patch_ReplaceWithBytes(target, kStolen, patch))
		{
			rslog::error_ts() << "CABINET_IR: hook write failed; rate patch remains applied" << std::endl;
			return false;
		}
		rslog::info_ts() << "CABINET_IR: resampler installed after media fetch at RVA 0x"
			<< std::hex << siteRva << std::dec << std::endl;
		return true;
	}
}

void InstallCabinetIRProbe(uintptr_t siteRva)
{
	if (GetGameSampleRate() != 44100)
	{
		rslog::info_ts() << "CABINET_IR: native 48000 Hz; cabinet resample not needed" << std::endl;
		return;
	}

	const ModuleImage img = GetGameImage();
	if (!img.base)
	{
		rslog::error_ts() << "CABINET_IR: game image unavailable" << std::endl;
		return;
	}
	InstallCabinetIRProbeAt(img, siteRva);
}
