# RS-ASIO for Rocksmith 2011 — Development Plan

## Goal

Port the RS-ASIO project to Rocksmith (2011) to enable low-latency audio on Linux via
Steam Proton + WineASIO, mirroring what the original project achieves for Rocksmith 2014.

## Background

**RS-ASIO for RS2014** works by:
1. Placing a fake `avrt.dll` in the game folder, which Windows loads instead of the
   system one (DLL side-loading). That DLL then loads `RS_ASIO.dll`.
2. `RS_ASIO.dll` patches the game executable in memory to redirect its calls to
   `CoCreateInstance` (the COM factory used to create WASAPI device enumerators) to our
   own function instead.
3. Our `Patched_CoCreateInstance` returns a fake `IMMDeviceEnumerator` that presents ASIO
   devices as if they were regular WASAPI devices.

**Rocksmith 2011 analysis findings:**
- Binary: `Rocksmith.exe`, 10,848,400 bytes, dated 2015-06-07
- Architecture: x86 (32-bit), PE32
- CRC32: `0xe0f686e0`
- Audio output: PortAudio compiled in statically, using the WASAPI backend
  → `MMDevAPI.dll` loads at runtime (WASAPI confirmed)
  → `avrt.dll` is probed from the game folder first → **same injection vector as RS2014**
- Audio input (guitar): **WASAPI exclusive mode**, not WinMM as initially suspected.
  The game identifies the Real Tone Cable specifically among all WASAPI capture devices
  by reading its property store, then opens it directly. The cable's native format is
  48 kHz / 16-bit mono, but Wine/Proton rejects all formats for it in exclusive mode
  under PipeWire. Solution: intercept the cable's `IMMDevice::Activate(IAudioClient)`
  call and return an ASIO-backed `RSAsioAudioClient` instead.
- COM imports (same PortAudio WASAPI pattern as RS2014):
  `CoCreateInstance`, `CoMarshalInterThreadInterfaceInStream`, `CoGetInterfaceAndReleaseStream`
- `avrt.dll` is probed from the game folder first → same injection vector as RS2014
- RS2011 uses WASAPI in **polling mode** (no `AUDCLNT_STREAMFLAGS_EVENTCALLBACK`),
  unlike RS2014 which uses event-driven mode.

**Process Monitor log:** `tools/rocksmith-processmonitor-log.CSV`  
**PE import inspector:** `tools/inspect_imports.ps1`  
**CRC32 calculator:** `tools/crc32.ps1`

---

## Status

| Phase | Status |
|-------|--------|
| Phase 0 — Linux baseline test | ✅ Done — game runs but latency is too high |
| Phase 1 — Tooling setup | ✅ Done |
| Phase 2a — Process Monitor trace | ✅ Done — WASAPI confirmed, `avrt.dll` is injection vector |
| Phase 2b — x32dbg: find `CoCreateInstance` call byte pattern | ⬜ Pending (requires restart) |
| Phase 2c — Ghidra: find PortAudio marshal/unmarshal byte patterns | ⬜ Pending |
| Phase 2d — Ghidra: assess WinMM input path | ⬜ Pending |
| Phase 3 — Define patch targets | ⬜ Pending |
| Phase 4 — Coding | ⬜ Pending |
| Phase 5 — Linux integration test | ⬜ Pending |

---

## Phase 0 — Linux Baseline Test ✅

Try running the game on Linux via Steam Proton before writing any code.

**Result:** Game runs but audio latency is too high. Confirms there is a real problem to
solve, and that the Proton + WinMM path alone is insufficient.

---

## Phase 1 — Tooling Setup ✅

All tools installed:

| Tool | Purpose |
|------|---------|
| **Ghidra** | Disassemble/decompile `Rocksmith.exe`; find byte patterns |
| **x32dbg** | 32-bit debugger; set breakpoints to observe runtime behavior |
| **PE-bear** | Inspect PE sections and imports without disassembly |
| **Process Monitor** (Sysinternals) | Trace DLL loads at runtime |
| **dumpbin.exe** | Inspect PE imports from the command line (in `tools/`) |

---

## Phase 2 — Runtime & Static Analysis

### Step 2a — Process Monitor trace ✅

**Findings:**
- `MMDevAPI.dll` loads from `SysWOW64` → WASAPI is actively used
- `avrt.dll` is searched in the game folder first (`NAME NOT FOUND`) → same injection
  vector as RS2014; placing a fake `avrt.dll` there will work
- `dsound.dll` is never loaded → DirectSound is not used
- `WINMM.dll` and `AUDIOSES.DLL` are also searched in the game folder first → could serve
  as alternative injection vectors if needed

### Step 2b — x32dbg: find `CoCreateInstance` byte patterns ⬜

This step identifies the exact machine code bytes around the game's call(s) to
`CoCreateInstance`, which become the `originalBytes_call_CoCreateInstance[]` arrays in
the new patcher file.

**Instructions:**
1. Open x32dbg and load `Rocksmith.exe` (or attach to a running instance).
2. Go to **Symbols → ole32.dll** and find `CoCreateInstance`. Set a breakpoint on it.
   Alternatively: in the command bar type `bp ole32.CoCreateInstance`.
3. Run the game until the breakpoint fires.
4. In the **Call Stack** panel, click the return address to jump to the call site inside
   `Rocksmith.exe`.
5. In the disassembly view, note the bytes at and around the `CALL` instruction:
   - If it looks like `FF 15 xx xx xx xx` → indirect call through an import table pointer
   - If it looks like `E8 xx xx xx xx` → direct relative call
6. Record **6–10 bytes** starting from the `CALL` instruction plus the bytes immediately
   following it (usually `TEST EAX, EAX` = `85 C0`). These bytes are unique enough to
   locate the call site with `FindBytesOffsets()`.
7. Check whether `CoCreateInstance` is called more than once (the call stack may fire
   multiple times). Record each distinct call site.
8. Also note the CLSID being passed at the first argument on the stack — confirm it is
   `BCDE0395-E52F-467C-8E3D-C4579291692E` (CLSID_MMDeviceEnumerator).

> **Tip:** In x32dbg, right-click on a byte sequence in the hex dump and choose
> "Copy → Copy bytes" to get the hex string.

### Step 2c — Ghidra: find PortAudio marshal/unmarshal byte patterns ⬜

RS_ASIO also patches two PortAudio-internal functions that marshal and unmarshal COM
interface pointers across threads. These patches prevent PortAudio from interfering with
the fake COM objects.

**Instructions:**
1. Import `Rocksmith.exe` into a new Ghidra project. Accept auto-analysis defaults.
   Note: the executable has a `PSFD00` section that may be a packer stub. If auto-analysis
   fails to decode large regions, this section unpacks code at load time; in that case,
   use x32dbg to dump the unpacked process memory and import the dump into Ghidra instead.
2. After analysis, open **Window → Symbol References** and search for:
   - `CoMarshalInterThreadInterfaceInStream`
   - `CoGetInterfaceAndReleaseStream`
3. For each reference, navigate to the call site and record the surrounding bytes
   (same approach as step 2b: 8–10 bytes from the `CALL`, unique enough to match).
4. Cross-reference the RS2014 patcher files for comparison:
   - [RS_ASIO/Patcher_21a8959a.cpp](../RS_ASIO/Patcher_21a8959a.cpp) — pre-2022 version
   - [RS_ASIO/Patcher_d1b38fcb.cpp](../RS_ASIO/Patcher_d1b38fcb.cpp) — post-2022 version
   The byte patterns will differ, but the function structure will be identical.

### Step 2d — Ghidra: assess WinMM input path ⬜

RS2011 routes guitar input through the WinMM Wave Input API (`waveInOpen` etc.) rather
than WASAPI. This may or may not be a problem under WineASIO on Linux.

**Instructions:**
1. In Ghidra, search for references to `waveInOpen`.
2. Trace the code path: is the device selected by the user (or hardcoded), and is there
   a device enumeration step that could limit choices?
3. On Linux with WineASIO, the WinMM input path routes through Wine's WinMM → PulseAudio
   or JACK. Test whether guitar input is audible at acceptable latency before attempting
   to patch this path.
4. If WinMM input latency is acceptable, no patch is needed for it.
   If it is not, a `waveInOpen` interception may be needed (significantly more complex than
   the WASAPI path — evaluate before committing to it).

---

## Phase 3 — Define Patch Targets

Based on Phase 2, create a summary table:

| Patch name | Byte pattern (hex) | Patch function | Notes |
|------------|-------------------|----------------|-------|
| `CoCreateInstance` call | TBD (from step 2b) | `Patched_CoCreateInstance` | Same as RS2014 |
| PortAudio MarshalStreamComPointers | TBD (from step 2c) | `Patched_PortAudio_MarshalStreamComPointers` | Same as RS2014 |
| PortAudio UnmarshalStreamComPointers | TBD (from step 2c) | `Patched_PortAudio_UnmarshalStreamComPointers` | Same as RS2014 |
| TwoRealToneCablesMessageBox | TBD | suppress dialog | May or may not exist in RS2011 |

---

## Phase 4 — Coding

The code changes are deliberately minimal. The entire ASIO wrapping layer, fake device
stack, INI configurator, and debug wrappers are **unchanged** from the original project.
Only the version-specific patch data is added.

### Step 4a — Add the RS2011 patcher file

Create `RS_ASIO/Patcher_e0f686e0.cpp` (named after the CRC32).

Structure it identically to the existing patcher files:

```cpp
// RS_ASIO/Patcher_e0f686e0.cpp
// Patch code for Rocksmith (2011) — single known version, CRC32 0xe0f686e0

#include "stdafx.h"
#include "dllmain.h"
#include "Patcher.h"

static const BYTE originalBytes_call_CoCreateInstance[]{
    // TBD from step 2b
};

static const BYTE originalBytes_call_PortAudio_MarshalStreamComPointers[]{
    // TBD from step 2c
};

static const BYTE originalBytes_call_UnmarshalStreamComPointers[]{
    // TBD from step 2c
};

void PatchOriginalCode_e0f686e0()
{
    // ... (same structure as PatchOriginalCode_21a8959a)
}
```

### Step 4b — Register the new version in the patcher dispatcher

In `RS_ASIO/Patcher.cpp`, in the `PatchOriginalCode()` function, add:

```cpp
case 0xe0f686e0:
    PatchOriginalCode_e0f686e0();
    break;
```

And add the forward declaration:

```cpp
void PatchOriginalCode_e0f686e0();
```

### Step 4c — Verify the `avrt.dll` injection DLL

The existing `avrt/avrt.cpp` and `avrt/dllmain.cpp` should work unchanged — they expose
the same `AvSetMmThreadCharacteristics` stub that PortAudio calls. Confirm by checking
the `avrt.def` exports against the functions that PortAudio actually imports.

### Step 4d — Build

Build using the existing Visual Studio solution `RS_ASIO.sln`. Output: `avrt.dll` and
`RS_ASIO.dll`. Copy both to the RS2011 game folder along with `RS_ASIO.ini`.

---

## Phase 5 — Linux Integration Test

1. Transfer `avrt.dll`, `RS_ASIO.dll`, and `RS_ASIO.ini` to the RS2011 game folder on
   the Linux machine: `~/.steam/.../steamapps/common/Rocksmith/`
2. Configure `RS_ASIO.ini` for WineASIO (same as the RS2014 Linux guide in
   [docs/linux/ubuntu_1204_lts.md](../docs/linux/ubuntu_1204_lts.md), but pointing to
   the RS2011 folder).
3. Run the game and inspect `RS_ASIO-log.txt` in the game folder.
4. Verify that the log shows the RS_ASIO version banner, WineASIO detection, and no
   "Unknown game version" error.
5. Test in-game: start a song, confirm guitar input is audible and latency is acceptable.

---

## Key Reference Files

| File | Purpose |
|------|---------|
| [RS_ASIO/Patcher.cpp](../RS_ASIO/Patcher.cpp) | Version dispatcher — add the RS2011 `case` here |
| [RS_ASIO/Patcher_21a8959a.cpp](../RS_ASIO/Patcher_21a8959a.cpp) | RS2014 pre-2022 patcher — model for RS2011 patcher |
| [RS_ASIO/dllmain.cpp](../RS_ASIO/dllmain.cpp) | The three patched functions live here |
| [RS_ASIO/dllmain.h](../RS_ASIO/dllmain.h) | Declarations of the three patched functions |
| [avrt/avrt.cpp](../avrt/avrt.cpp) | The fake avrt.dll that bootstraps RS_ASIO.dll |
| [docs/linux/ubuntu_1204_lts.md](../docs/linux/ubuntu_1204_lts.md) | Linux setup guide (RS2014) — RS2011 setup will be similar |
| tools/rocksmith-processmonitor-log.CSV | Process Monitor trace of RS2011 startup |
| tools/inspect_imports.ps1 | Lists all PE imports from Rocksmith.exe |
| tools/crc32.ps1 | Computes CRC32 of any PE file |
