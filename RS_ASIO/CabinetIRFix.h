#pragma once

#include <cstdint>

// The cabinet effect rejects impulse responses authored at a different rate.
// Resample its already-fetched media in place at load time, before that check.
// Installs only for GameSampleRate=44100; siteRva is specific to the game build.
void InstallCabinetIRProbe(uintptr_t siteRva);
