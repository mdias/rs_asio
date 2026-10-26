#pragma once

#include <cstdint>

// The cabinet effect rejects impulse responses authored at a different rate.
// Resample its already-fetched media in place at load time, before that check.
// Installs only when GetGameSampleRate() selects 44100 Hz; siteRva is build-specific.
void InstallCabinetIRProbe(uintptr_t siteRva);
