#include "stdafx.h"
#include <limits>
#include <cstdint>
#include <cstring>

#include "AudioProcessing.h"

using TFuncConvert = void(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples);
using TFuncConvertMatrix = std::array<std::array<TFuncConvert*, ASIOSTLastEntry>, ASIOSTLastEntry>;


static const TFuncConvertMatrix& GetFuncConvertMatrix();

template<unsigned inSize, typename TIntRet>
static TIntRet ReadAudioSampleInt(const BYTE* inData)
{
	if (inSize < sizeof(TIntRet))
	{
		TIntRet val = 0;
		BYTE* outData = (BYTE*)&val;

		unsigned i = 0;
		for (; i < inSize; ++i)
		{
			outData[i] = inData[i];
		}
		for (; i < sizeof(TIntRet); ++i)
		{
			val <<= 8;
		}

		return val;
	}
	else if (inSize > sizeof(TIntRet))
	{
		const int excessSize = inSize - sizeof(TIntRet);
		inData += excessSize;
		return *(TIntRet*)inData;
	}
	else
	{
		return *(TIntRet*)inData;
	}
}

template<unsigned outSize, typename TInt>
static void WriteAudioSampleInt(TInt sample, BYTE* outData)
{
	const BYTE* sampleBytes = (BYTE*)&sample;

	if (outSize < sizeof(TInt))
	{
		const int excessSize = sizeof(TInt) - outSize;
		sampleBytes += excessSize;

		for (unsigned i = 0; i < outSize; ++i)
		{
			outData[i] = sampleBytes[i];
		}
	}
	else if (outSize > sizeof(TInt))
	{
		const int excessSize = outSize - sizeof(TInt);
		outData += excessSize;

		for (unsigned i = 0; i < sizeof(TInt); ++i)
		{
			outData[i] = sampleBytes[i];
		}
	}
	else
	{
		*reinterpret_cast<TInt*>(outData) = sample;
	}
}

static bool AudioBlit(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const unsigned bytesPerSample, const DWORD numSamples)
{
	if (bytesPerSample == 1)
	{
		for (DWORD i = 0; i < numSamples; ++i)
		{
			*outData = *inData;
			inData += inStride;
			outData += outStride;
		}
		return true;
	}
	else if (bytesPerSample == 2)
	{
		for (DWORD i = 0; i < numSamples; ++i)
		{
			*reinterpret_cast<int16_t*>(outData) = *reinterpret_cast<const int16_t*>(inData);
			inData += inStride;
			outData += outStride;
		}
		return true;
	}
	else if (bytesPerSample == 3)
	{
		for (DWORD i = 0; i < numSamples; ++i)
		{
			*reinterpret_cast<int16_t*>(outData) = *reinterpret_cast<const int16_t*>(inData);
			outData[2] = inData[2];
			inData += inStride;
			outData += outStride;
		}
		return true;
	}
	else if (bytesPerSample == 4)
	{
		for (DWORD i = 0; i < numSamples; ++i)
		{
			*reinterpret_cast<int32_t*>(outData) = *reinterpret_cast<const int32_t*>(inData);
			inData += inStride;
			outData += outStride;
		}
		return true;
	}
	else if (bytesPerSample == 8)
	{
		for (DWORD i = 0; i < numSamples; ++i)
		{
			*reinterpret_cast<int64_t*>(outData) = *reinterpret_cast<const int64_t*>(inData);
			inData += inStride;
			outData += outStride;
		}
		return true;
	}

	return false;
}

template<unsigned inSampleSize, unsigned outSampleSize>
static void AudioCopyConvert(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples)
{
	using TSampleType = typename std::conditional<inSampleSize == 2,
		std::int16_t,
		std::int32_t>::type;

	for (DWORD i = 0; i < numSamples; ++i)
	{
		const auto sample = ReadAudioSampleInt<inSampleSize, TSampleType>(inData);
		WriteAudioSampleInt<outSampleSize>(sample, outData);
		inData += inStride;
		outData += outStride;
	}
}

template<typename TFloatType, unsigned outSampleSize>
static void AudioCopyConvertF2I(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples)
{
	const auto Max32 = (std::numeric_limits<std::int32_t>::max)() - 1;
	const double Max32_d = (double)Max32;

	for (DWORD i = 0; i < numSamples; ++i)
	{
		const double sample_f = *(const TFloatType*)inData * Max32_d;
		const std::int32_t sample_i = (std::int32_t)sample_f;
		WriteAudioSampleInt<outSampleSize>(sample_i, outData);
		inData += inStride;
		outData += outStride;
	}
}

template<unsigned inSampleSize, typename TFloatType>
static void AudioCopyConvertI2F(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples)
{
	using TSampleType = typename std::conditional<inSampleSize == 2,
		std::int16_t,
		std::int32_t>::type;

	const auto SampleTypeMax = (std::numeric_limits<TSampleType>::max)();
	const auto SampleTypeMax_d = 1.0 / (double)SampleTypeMax;

	for (DWORD i = 0; i < numSamples; ++i)
	{
		const auto sample_i = ReadAudioSampleInt<inSampleSize, TSampleType>(inData);
		const double sample_f = (double)sample_i * SampleTypeMax_d;
		*(TFloatType*)outData = (TFloatType)sample_f;
		inData += inStride;
		outData += outStride;
	}
}

template<typename TInFloatType, typename TOutFloatType>
static void AudioCopyConvertF2F(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples)
{
	for (DWORD i = 0; i < numSamples; ++i)
	{
		const auto sample = *(const TInFloatType*)inData;
		*(TOutFloatType*)outData = (TOutFloatType)sample;
		inData += inStride;
		outData += outStride;
	}
}

bool AudioProcessing::CopyConvertFormat(const BYTE* inData, ASIOSampleType inSampleType, const WORD inStride, DWORD numFrames, BYTE* outData, ASIOSampleType outSampleType, const WORD outStride)
{
	const WORD inSampleSize = GetAsioSampleTypeNumBytes(inSampleType);
	const WORD outSampleSize = GetAsioSampleTypeNumBytes(outSampleType);

	if (!inSampleSize || !outSampleSize)
		return false;

	// simple copy case; no conversion needed here
	if (inSampleType == outSampleType)
	{
		return AudioBlit(inData, inStride, outData, outStride, inSampleSize, numFrames);
	}
	else
	{
		// get convert function to use
		const auto& convFuncMatrix = GetFuncConvertMatrix();
		auto convFunc = convFuncMatrix[inSampleType][outSampleType];
		if (!convFunc)
			return false;
		convFunc(inData, inStride, outData, outStride, numFrames);

		return true;
	}
}

bool AudioProcessing::CanConvertFormat(ASIOSampleType inSampleType, ASIOSampleType outSampleType)
{
	if (!GetAsioSampleTypeNumBytes(inSampleType) || !GetAsioSampleTypeNumBytes(outSampleType))
		return false;
	return inSampleType == outSampleType || GetFuncConvertMatrix()[inSampleType][outSampleType] != nullptr;
}

bool AudioProcessing::DoSoftwareVolumeDsp(BYTE* data, ASIOSampleType inSampleType, DWORD numSamples, float fVolumeScalar)
{
	if (inSampleType == ASIOSTInt16LSB)
	{
		const int scalarPercentPoints = (int)(fVolumeScalar * 100.f);
		for (DWORD i = 0; i < numSamples; ++i, data += 2)
		{
			std::int64_t sample = *(std::int16_t*)data;
			sample = (sample * scalarPercentPoints) / 100;
			*(std::int16_t*)data = (std::int16_t)sample;
		}
		return true;
	}
	else if (inSampleType == ASIOSTInt24LSB)
	{
		const int scalarPercentPoints = (int)(fVolumeScalar * 100.f);
		for (DWORD i = 0; i < numSamples; ++i, data += 3)
		{
			std::int64_t sample = data[0] | (data[1] << 8) | (data[2] << 16);
			if (sample & 0x800000)
				sample |= ~std::int64_t(0xffffff);
			sample = (sample * scalarPercentPoints) / 100;
			data[0] = (std::uint8_t)(sample & 0xff);
			data[1] = (std::uint8_t)((sample >> 8) & 0xff);
			data[2] = (std::uint8_t)((sample >> 16) & 0xff);
		}
		return true;
	}
	else if (inSampleType == ASIOSTInt32LSB)
	{
		const int scalarPercentPoints = (int)(fVolumeScalar * 100.f);
		for (DWORD i = 0; i < numSamples; ++i, data += 4)
		{
			std::int64_t sample = *(std::int32_t*)data;
			sample = (sample * scalarPercentPoints) / 100;
			*(std::int32_t*)data = (std::int32_t)sample;
		}
		return true;
	}
	else if (inSampleType == ASIOSTFloat32LSB)
	{
		for (DWORD i = 0; i < numSamples; ++i, data += 4)
		{
			(*(float*)data) *= fVolumeScalar;
		}
		return true;
	}
	else if (inSampleType == ASIOSTFloat64LSB)
	{
		const double v = (double)fVolumeScalar;
		for (DWORD i = 0; i < numSamples; ++i, data += 8)
		{
			(*(double*)data) *= v;
		}
		return true;
	}
	return false;
}

// PCM containers carry right-aligned valid bits; writes sign-extend the value.
template <ASIOSampleType T, unsigned Bytes, bool IsFloat, bool BigEndian, unsigned Bits>
struct SampleCodec
{
	static constexpr ASIOSampleType type = T;
	static constexpr unsigned bytes = Bytes;

	static std::uint64_t Load(const BYTE* p)
	{
		std::uint64_t v = 0;
		for (unsigned i = 0; i < Bytes; ++i)
			v |= static_cast<std::uint64_t>(p[BigEndian ? i : Bytes - 1 - i]) << (8 * (Bytes - 1 - i));
		return v;
	}
	static void Store(BYTE* p, std::uint64_t v)
	{
		for (unsigned i = 0; i < Bytes; ++i)
			p[BigEndian ? i : Bytes - 1 - i] = static_cast<BYTE>(v >> (8 * (Bytes - 1 - i)));
	}

	static double Read(const BYTE* p)
	{
		const std::uint64_t raw = Load(p);
		if constexpr (IsFloat)
		{
			if (Bytes == 4) { std::uint32_t u = static_cast<std::uint32_t>(raw); float f; std::memcpy(&f, &u, 4); return f; }
			double d; std::memcpy(&d, &raw, 8); return d;
		}
		else
		{
			// Sign-extend the low Bits of the container, then normalise.
			const unsigned shift = 64 - Bits;
			const std::int64_t value = static_cast<std::int64_t>(raw << shift) >> shift;
			return static_cast<double>(value) / static_cast<double>(std::int64_t(1) << (Bits - 1));
		}
	}
	static void Write(BYTE* p, double x)
	{
		if constexpr (IsFloat)
		{
			if (Bytes == 4) { const float f = static_cast<float>(x); std::uint32_t u; std::memcpy(&u, &f, 4); Store(p, u); return; }
			std::uint64_t u; std::memcpy(&u, &x, 8); Store(p, u); return;
		}
		else
		{
			const double scale = static_cast<double>(std::int64_t(1) << (Bits - 1));
			const double maxValue = scale - 1.0;
			double scaled = x * scale;
			std::uint64_t representation;
			std::memcpy(&representation, &x, sizeof(representation));
			if ((representation & 0x7fffffffffffffffull) > 0x7ff0000000000000ull)
				scaled = 0.0; // NaN -> silence, including /fp:fast builds
			scaled = scaled < -scale ? -scale : (scaled > maxValue ? maxValue : scaled);
			const std::int64_t value = static_cast<std::int64_t>(scaled < 0.0 ? scaled - 0.5 : scaled + 0.5);
			const std::int64_t clamped = value < -static_cast<std::int64_t>(scale) ? -static_cast<std::int64_t>(scale)
				: (value > static_cast<std::int64_t>(maxValue) ? static_cast<std::int64_t>(maxValue) : value);
			Store(p, static_cast<std::uint64_t>(clamped));
		}
	}
};

using CodecInt16MSB = SampleCodec<ASIOSTInt16MSB, 2, false, true, 16>;
using CodecInt24MSB = SampleCodec<ASIOSTInt24MSB, 3, false, true, 24>;
using CodecInt32MSB = SampleCodec<ASIOSTInt32MSB, 4, false, true, 32>;
using CodecFloat32MSB = SampleCodec<ASIOSTFloat32MSB, 4, true, true, 32>;
using CodecFloat64MSB = SampleCodec<ASIOSTFloat64MSB, 8, true, true, 64>;
using CodecInt32MSB16 = SampleCodec<ASIOSTInt32MSB16, 4, false, true, 16>;
using CodecInt32MSB18 = SampleCodec<ASIOSTInt32MSB18, 4, false, true, 18>;
using CodecInt32MSB20 = SampleCodec<ASIOSTInt32MSB20, 4, false, true, 20>;
using CodecInt32MSB24 = SampleCodec<ASIOSTInt32MSB24, 4, false, true, 24>;
using CodecInt16LSB = SampleCodec<ASIOSTInt16LSB, 2, false, false, 16>;
using CodecInt24LSB = SampleCodec<ASIOSTInt24LSB, 3, false, false, 24>;
using CodecInt32LSB = SampleCodec<ASIOSTInt32LSB, 4, false, false, 32>;
using CodecFloat32LSB = SampleCodec<ASIOSTFloat32LSB, 4, true, false, 32>;
using CodecFloat64LSB = SampleCodec<ASIOSTFloat64LSB, 8, true, false, 64>;
using CodecInt32LSB16 = SampleCodec<ASIOSTInt32LSB16, 4, false, false, 16>;
using CodecInt32LSB18 = SampleCodec<ASIOSTInt32LSB18, 4, false, false, 18>;
using CodecInt32LSB20 = SampleCodec<ASIOSTInt32LSB20, 4, false, false, 20>;
using CodecInt32LSB24 = SampleCodec<ASIOSTInt32LSB24, 4, false, false, 24>;

template <class... Codecs> struct CodecList {};
using AllPcmCodecs = CodecList<
	CodecInt16MSB, CodecInt24MSB, CodecInt32MSB, CodecFloat32MSB, CodecFloat64MSB,
	CodecInt32MSB16, CodecInt32MSB18, CodecInt32MSB20, CodecInt32MSB24,
	CodecInt16LSB, CodecInt24LSB, CodecInt32LSB, CodecFloat32LSB, CodecFloat64LSB,
	CodecInt32LSB16, CodecInt32LSB18, CodecInt32LSB20, CodecInt32LSB24>;

template <class In, class Out>
static void AudioCopyConvertGeneric(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples)
{
	for (DWORD i = 0; i < numSamples; ++i, inData += inStride, outData += outStride)
		Out::Write(outData, In::Read(inData));
}

template <class In, class... Outs>
static void FillGenericRow(TFuncConvertMatrix& m, CodecList<Outs...>)
{
	((m[In::type][Outs::type] ? void() : void(m[In::type][Outs::type] = &AudioCopyConvertGeneric<In, Outs>)), ...);
}

template <class... Ins>
static void FillGenericMatrix(TFuncConvertMatrix& m, CodecList<Ins...> all)
{
	(FillGenericRow<Ins>(m, all), ...);
}

static const TFuncConvertMatrix& GetFuncConvertMatrix()
{
	static const TFuncConvertMatrix matrix = [] {
		TFuncConvertMatrix result{};

		result[ASIOSTInt16LSB][ASIOSTInt16LSB] = &AudioCopyConvert<2, 2>;
		result[ASIOSTInt16LSB][ASIOSTInt24LSB] = &AudioCopyConvert<2, 3>;
		result[ASIOSTInt16LSB][ASIOSTInt32LSB] = &AudioCopyConvert<2, 4>;
		result[ASIOSTInt16LSB][ASIOSTFloat32LSB] = &AudioCopyConvertI2F<2, float>;
		result[ASIOSTInt16LSB][ASIOSTFloat64LSB] = &AudioCopyConvertI2F<2, double>;

		result[ASIOSTInt24LSB][ASIOSTInt16LSB] = &AudioCopyConvert<3, 2>;
		result[ASIOSTInt24LSB][ASIOSTInt24LSB] = &AudioCopyConvert<3, 3>;
		result[ASIOSTInt24LSB][ASIOSTInt32LSB] = &AudioCopyConvert<3, 4>;
		result[ASIOSTInt24LSB][ASIOSTFloat32LSB] = &AudioCopyConvertI2F<3, float>;
		result[ASIOSTInt24LSB][ASIOSTFloat64LSB] = &AudioCopyConvertI2F<3, double>;

		result[ASIOSTInt32LSB][ASIOSTInt16LSB] = &AudioCopyConvert<4, 2>;
		result[ASIOSTInt32LSB][ASIOSTInt24LSB] = &AudioCopyConvert<4, 3>;
		result[ASIOSTInt32LSB][ASIOSTInt32LSB] = &AudioCopyConvert<4, 4>;
		result[ASIOSTInt32LSB][ASIOSTFloat32LSB] = &AudioCopyConvertI2F<4, float>;
		result[ASIOSTInt32LSB][ASIOSTFloat64LSB] = &AudioCopyConvertI2F<4, double>;

		result[ASIOSTFloat32LSB][ASIOSTInt16LSB] = &AudioCopyConvertF2I<float, 2>;
		result[ASIOSTFloat32LSB][ASIOSTInt24LSB] = &AudioCopyConvertF2I<float, 3>;
		result[ASIOSTFloat32LSB][ASIOSTInt32LSB] = &AudioCopyConvertF2I<float, 4>;
		result[ASIOSTFloat32LSB][ASIOSTFloat32LSB] = &AudioCopyConvertF2F<float, float>;
		result[ASIOSTFloat32LSB][ASIOSTFloat64LSB] = &AudioCopyConvertF2F<float, double>;

		result[ASIOSTFloat64LSB][ASIOSTInt16LSB] = &AudioCopyConvertF2I<double, 2>;
		result[ASIOSTFloat64LSB][ASIOSTInt24LSB] = &AudioCopyConvertF2I<double, 3>;
		result[ASIOSTFloat64LSB][ASIOSTInt32LSB] = &AudioCopyConvertF2I<double, 4>;
		result[ASIOSTFloat64LSB][ASIOSTFloat32LSB] = &AudioCopyConvertF2F<double, float>;
		result[ASIOSTFloat64LSB][ASIOSTFloat64LSB] = &AudioCopyConvertF2F<double, double>;
		FillGenericMatrix(result, AllPcmCodecs{});
		return result;
	}();

	return matrix;
}
