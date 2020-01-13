#include "stdafx.h"
#include <limits>

#include "AudioProcessing.h"

using TFuncConvert = void(const BYTE* inData, const WORD inStride, BYTE* outData, const WORD outStride, const DWORD numSamples);
using TFuncConvertMatrix = std::array<std::array<TFuncConvert*, ASIOSTLastEntry>, ASIOSTLastEntry>;

static TFuncConvertMatrix s_FuncConvertMatrix{};

static const TFuncConvertMatrix& GetFuncConvertMatrix();

template<unsigned bytes, typename TIntRet>
static TIntRet ReadAudioSampleInt(const BYTE* inData)
{
	if (bytes != sizeof(TIntRet))
	{
		TIntRet val = 0;

		unsigned i = 0;
		for (; i < bytes; ++i)
		{
			val <<= 8;
			val |= inData[i];
		}
		for (; i < sizeof(TIntRet); ++i)
		{
			val <<= 8;
		}

		return val;
	}
	else
	{
		return *(TIntRet*)inData;
	}
}

template<unsigned outSize, typename TInt>
static void WriteAudioSampleInt(TInt sample, BYTE* outData)
{
	if (outSize < sizeof(TInt))
	{
		const int excessSize = sizeof(TInt) - outSize;
		sample >>= 8 * excessSize;

		for (unsigned i = 0; i < outSize; ++i)
		{
			outData[outSize-i-1] = sample & 0xff;
			sample >>= 8;
		}
	}
	else if (outSize > sizeof(TInt))
	{
		unsigned i = 0;
		for (; i < sizeof(TInt); ++i)
		{
			outData[sizeof(TInt) - i - 1] = sample & 0xff;
			sample >>= 8;
		}
		for (; i < outSize; ++i)
		{
			outData[i] = 0x00;
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

	if (!inSampleType || !outSampleType)
		return false;

	// simple copy case; no conversion needed here
	if (inSampleType == outSampleType)
	{
		AudioBlit(inData, inStride, outData, outStride, inSampleSize, numFrames);
		return true;
	}
	else
	{
		// get convert function to use
		const auto& convFuncMatrix = GetFuncConvertMatrix();
		auto convFunc = convFuncMatrix[inSampleType][outSampleType];
		if (convFunc)
		{
			// call conversion function
			convFunc(inData, inStride, outData, outStride, numFrames);
		}

		return true;
	}
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
			std::int64_t sample = *(std::int16_t*)data;
			sample = (sample << 8) | data[2];
			sample = (sample * scalarPercentPoints) / 100;
			*(std::int16_t*)data = (std::int16_t)(sample >> 8);
			data[2] = sample & 0xff;
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

static const TFuncConvertMatrix& GetFuncConvertMatrix()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		s_FuncConvertMatrix[ASIOSTInt16LSB][ASIOSTInt16LSB] = &AudioCopyConvert<2, 2>;
		s_FuncConvertMatrix[ASIOSTInt16LSB][ASIOSTInt24LSB] = &AudioCopyConvert<2, 3>;
		s_FuncConvertMatrix[ASIOSTInt16LSB][ASIOSTInt32LSB] = &AudioCopyConvert<2, 4>;
		s_FuncConvertMatrix[ASIOSTInt16LSB][ASIOSTFloat32LSB] = &AudioCopyConvertI2F<2, float>;
		s_FuncConvertMatrix[ASIOSTInt16LSB][ASIOSTFloat64LSB] = &AudioCopyConvertI2F<2, double>;

		s_FuncConvertMatrix[ASIOSTInt24LSB][ASIOSTInt16LSB] = &AudioCopyConvert<3, 2>;
		s_FuncConvertMatrix[ASIOSTInt24LSB][ASIOSTInt24LSB] = &AudioCopyConvert<3, 3>;
		s_FuncConvertMatrix[ASIOSTInt24LSB][ASIOSTInt32LSB] = &AudioCopyConvert<3, 4>;
		s_FuncConvertMatrix[ASIOSTInt24LSB][ASIOSTFloat32LSB] = &AudioCopyConvertI2F<3, float>;
		s_FuncConvertMatrix[ASIOSTInt24LSB][ASIOSTFloat64LSB] = &AudioCopyConvertI2F<3, double>;

		s_FuncConvertMatrix[ASIOSTInt32LSB][ASIOSTInt16LSB] = &AudioCopyConvert<4, 2>;
		s_FuncConvertMatrix[ASIOSTInt32LSB][ASIOSTInt24LSB] = &AudioCopyConvert<4, 3>;
		s_FuncConvertMatrix[ASIOSTInt32LSB][ASIOSTInt32LSB] = &AudioCopyConvert<4, 4>;
		s_FuncConvertMatrix[ASIOSTInt32LSB][ASIOSTFloat32LSB] = &AudioCopyConvertI2F<4, float>;
		s_FuncConvertMatrix[ASIOSTInt32LSB][ASIOSTFloat64LSB] = &AudioCopyConvertI2F<5, double>;

		s_FuncConvertMatrix[ASIOSTFloat32LSB][ASIOSTInt16LSB] = &AudioCopyConvertF2I<float, 2>;
		s_FuncConvertMatrix[ASIOSTFloat32LSB][ASIOSTInt24LSB] = &AudioCopyConvertF2I<float, 3>;
		s_FuncConvertMatrix[ASIOSTFloat32LSB][ASIOSTInt32LSB] = &AudioCopyConvertF2I<float, 4>;
		s_FuncConvertMatrix[ASIOSTFloat32LSB][ASIOSTFloat32LSB] = &AudioCopyConvertF2F<float, float>;
		s_FuncConvertMatrix[ASIOSTFloat32LSB][ASIOSTFloat64LSB] = &AudioCopyConvertF2F<float, double>;

		s_FuncConvertMatrix[ASIOSTFloat64LSB][ASIOSTInt16LSB] = &AudioCopyConvertF2I<double, 2>;
		s_FuncConvertMatrix[ASIOSTFloat64LSB][ASIOSTInt24LSB] = &AudioCopyConvertF2I<double, 3>;
		s_FuncConvertMatrix[ASIOSTFloat64LSB][ASIOSTInt32LSB] = &AudioCopyConvertF2I<double, 4>;
		s_FuncConvertMatrix[ASIOSTFloat64LSB][ASIOSTFloat32LSB] = &AudioCopyConvertF2F<double, float>;
		s_FuncConvertMatrix[ASIOSTFloat64LSB][ASIOSTFloat64LSB] = &AudioCopyConvertF2F<double, double>;
	}

	return s_FuncConvertMatrix;
}