#pragma once

#include <algorithm>
#include <cmath>

// Endpoint-volume taper.
//
// Rocksmith calibrates its guitar input gain by writing scalars to the capture
// endpoint's IAudioEndpointVolume (observed call site Rocksmith2014.exe+0xa73404:
// 0.17, then 1.0, then a 0.40..0.50 search ramp). RS_ASIO used to treat that
// scalar as a raw linear amplitude multiplier, which is not what the interface
// means: a scalar is a position on a fader, mapped to dB through a taper.
//
// Measured against a real Windows endpoint on 2026-08-09 with
// a MOTU M Series (In 1-2 and Out 1-2):
//
//   scalar  windows dB   linear dB   error
//   0.05     -45.02       -26.02     +19.0
//   0.10     -34.75       -20.00     +14.8
//   0.17     -26.80       -15.39     +11.4   <- Rocksmith's calibration value
//   0.40     -13.89        -7.96      +5.9
//   0.50     -10.51        -6.02      +4.5
//   0.79      -3.58        -2.05      +1.5
//   1.00      +0.00        +0.00       0.0
//
// The old linear behaviour was therefore up to 19 dB too loud, and the game's
// closed calibration loop ran against a fader that disobeyed its own contract.
//
// The measured curve is a power law in amplitude: amplitude = scalar^1.74.
// Solving each measured point for the exponent gives 1.730 at scalar 0.05
// rising to 1.749 at 0.90 - a systematic drift, so this is a fit rather than
// the exact curve Windows uses. Worst-case deviation across the measured range
// is under 0.3 dB, comfortably inside the control's own 1.5 dB step, so the fit
// is well within the resolution the interface promises.
//
// The published range deliberately tops out at 0 dB. Both the Realtek onboard
// endpoint (+12.00 dB) and the ASIO SDK's own convention (ASIOInputMonitor.gain,
// "-inf to +12 dB") would justify positive makeup gain, but the game seeks a
// target level that the player matches with the interface's analog gain knob.
// Digital makeup above unity buys nothing the knob does not and costs headroom -
// capping at unity also keeps the integer volume path unable to overflow.
namespace VolumeTaper
{
	// Mirrors what a real Windows software endpoint publishes. QueryHardwareSupport
	// reports 0 alongside this: the gain is applied in our DSP, not in hardware.
	constexpr float kMinDb = -96.0f;
	constexpr float kMaxDb = 0.0f;
	constexpr float kStepDb = 1.5f;
	constexpr unsigned kStepCount = 51;

	// amplitude = scalar ^ kExponent, i.e. dB = 20 * kExponent * log10(scalar).
	constexpr float kExponent = 1.74f;

	// The scalar at which the taper reaches kMinDb. Below this the advertised
	// range has nothing left to express, so both the reported dB and the applied
	// amplitude must agree on silence - otherwise a caller reads -96 dB while the
	// DSP keeps attenuating (and eventually into denormals).
	// 10^(kMinDb / (20 * kExponent)) ~= 0.0017433.
	inline float FloorScalar() noexcept
	{
		return std::pow(10.0f, kMinDb / (20.0f * kExponent));
	}

	// A scalar of exactly 0 is silence, which is -inf dB; the interface reports
	// the bottom of the range instead, so callers get a finite number.
	// The `!(scalar > 0.0f)` form is deliberate: it is true for NaN, so NaN maps
	// to the bottom of the range rather than falling through the comparisons.
	inline float ScalarToDb(float scalar) noexcept
	{
		if (!(scalar > 0.0f))
			return kMinDb;
		if (scalar >= 1.0f)
			return kMaxDb;

		const float db = 20.0f * kExponent * std::log10(scalar);
		return (std::max)(kMinDb, (std::min)(kMaxDb, db));
	}

	inline float DbToScalar(float db) noexcept
	{
		if (db <= kMinDb)
			return 0.0f;
		if (db >= kMaxDb)
			return 1.0f;

		return std::pow(10.0f, db / (20.0f * kExponent));
	}

	// What the DSP actually multiplies samples by. Deliberately not the scalar.
	//
	// Anything at or below the floor becomes exactly 0: it keeps the applied gain
	// consistent with the -96 dB the getters report, and it keeps subnormal
	// amplitudes (reachable from legal scalars around 1e-23) off the audio path,
	// where their cost depends on an MXCSR FTZ/DAZ state we do not control.
	inline float ScalarToAmplitude(float scalar) noexcept
	{
		if (!(scalar > 0.0f))
			return 0.0f;
		if (scalar >= 1.0f)
			return 1.0f;
		if (scalar <= FloorScalar())
			return 0.0f;

		return std::pow(scalar, kExponent);
	}

	// The control advertises kStepDb granularity, so an applied dB value must
	// land on one of its discrete settings. Callers may still read back the
	// value they requested; this is what actually gets applied.
	inline float QuantizeDb(float db) noexcept
	{
		if (!(db > kMinDb))
			return kMinDb;
		if (db >= kMaxDb)
			return kMaxDb;

		const float stepsFromTop = (kMaxDb - db) / kStepDb;
		const float snapped = kMaxDb - std::floor(stepsFromTop + 0.5f) * kStepDb;
		return (std::max)(kMinDb, (std::min)(kMaxDb, snapped));
	}

	// The interface exposes volume as one of kStepCount discrete positions, so
	// VolumeStepUp/Down have somewhere well-defined to move to.
	inline unsigned ScalarToStep(float scalar) noexcept
	{
		const float clamped = (std::max)(0.0f, (std::min)(1.0f, scalar));
		const float step = clamped * static_cast<float>(kStepCount - 1);
		return static_cast<unsigned>(step + 0.5f);
	}

	inline float StepToScalar(unsigned step) noexcept
	{
		if (step >= kStepCount - 1)
			return 1.0f;
		return static_cast<float>(step) / static_cast<float>(kStepCount - 1);
	}
}
