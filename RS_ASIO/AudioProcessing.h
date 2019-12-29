#pragma once

namespace AudioProcessing
{
	bool CopyConvertFormat(const BYTE* inData, ASIOSampleType inSampleType, const WORD inStride, DWORD numFrames, BYTE* outData, ASIOSampleType outSampleType, const WORD outStride);
	bool DoSoftwareVolumeDsp(BYTE* data, ASIOSampleType inSampleType, DWORD numSamples, float fVolumeScalar);
}