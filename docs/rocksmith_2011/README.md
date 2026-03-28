# Rocksmith 2011 on Linux with RS ASIO

This guide covers running **Rocksmith 2011** on Linux via Steam/Proton with RS ASIO and WineASIO,
using the Real Tone Cable for guitar/bass input.

Support for Rocksmith 2011 was contributed by [ferabreu](https://github.com/ferabreu), developed
with the assistance of GitHub Copilot (Claude Sonnet 4.6 and Claude Haiku 4.5).

---

## Requirements

- The **Real Tone Cable** (Ubisoft USB guitar adapter) physically connected
- `Rocksmith.ini` set to `ExclusiveMode=1` (do **not** set `Win32UltraLowLatencyMode=1` — RS2011 does not support it)
- Follow the [Linux guide section](../linux/README.md) to set up RS ASIO and WineASIO for Rocksmith (these instructions apply to both 2014 and 2011)

---

## RS_ASIO.ini configuration

The default `dist/RS_ASIO.ini` file requires the following additional configuration for Rocksmith 2011:

```ini
[Config]
EnableWasapiInputs=1   ; required — enables WASAPI capture device enumeration

[Asio.Output]
Driver=wineasio-rsasio

[Asio.Input.0]
Driver=wineasio-rsasio
WasapiDevice=Rocksmith  ; matches the Real Tone Cable's friendly name on Wine/Proton
```

### WasapiDevice matching

The `WasapiDevice=` value is matched **case-insensitively** against both:
- The full WASAPI device ID string (e.g. `{0.0.1.00000000}.{21D5646C-D708-4E90-A57A-E1956015D4F3}`)
- The device's friendly name (e.g. `Rocksmith Guitar Adapter Mono`)

The value `Rocksmith` matches all known Real Tone Cable variants on Wine/Proton by friendly name.

If this does not match on your system, run the game once, open `RS_ASIO-log.txt`, and look for
lines like:

```
{0.0.1.00000000}.{21D5646C-...} friendly name: "Rocksmith Guitar Adapter Mono"
```

Use any substring of the friendly name, or a fragment of the GUID, as the `WasapiDevice=` value.

### ASIO channel assignment

`Channel=0` in `[Asio.Input.0]` maps to WineASIO's first input channel (`in_1`). Route your
guitar/bass interface into that channel in your PipeWire/JACK graph (e.g. via `qpwgraph` or
`carla`).

---

## Rocksmith.ini

RS2011's `Rocksmith.ini` (in the game folder) must have:

```ini
[Audio]
ExclusiveMode=1
```

Do **not** add `Win32UltraLowLatencyMode=1` — that setting is RS2014-specific and will break
RS2011's audio initialisation.

---

## Troubleshooting

**Game asks to connect the Real Tone Cable at the tuner screen**

The WASAPI redirect did not activate. Check `RS_ASIO-log.txt` for a line reading:
```
DebugWrapperDevice::Activate - redirecting IAudioClient to ASIO input
```
If it is absent, the `WasapiDevice=` value did not match any enumerated capture device. See
the [WasapiDevice matching](#wasapidevice-matching) section above.

**No audio output**

Verify WineASIO is installed and the `Driver=wineasio-rsasio` name matches what appears in
the `RS_ASIO-log.txt` under `AsioHelpers::FindDrivers`.

**Crackling or dropouts**

If you experience dropouts, adjust your PipeWire quantum and buffer settings. The typical setup
uses 256 frames at 48kHz (5ms latency), set via `PIPEWIRE_LATENCY="256/48000"` — but these are
recommendations, not hard constraints. You may need to increase buffer size if dropouts persist.

---

## How RS2011 audio differs from RS2014

Understanding these differences explains why extra configuration is needed.

### Output

Both games use WASAPI exclusive mode for audio output. RS ASIO intercepts the WASAPI device
enumeration and injects fake WASAPI devices backed by ASIO. This mechanism works identically
for both games — no special configuration is needed for output.

### Guitar input

This is where RS2011 and RS2014 diverge fundamentally.

**RS2014** uses WASAPI exclusive mode for the Real Tone Cable but does so through normal COM
device enumeration, which RS ASIO already intercepts and redirects to ASIO.

**RS2011** uses WASAPI exclusive mode in **polling mode** — it does not request event-driven
callbacks (`AUDCLNT_STREAMFLAGS_EVENTCALLBACK`), instead calling `GetCurrentPadding` in a loop
to detect when new audio data is available. Under Wine/Proton, this combination causes RS ASIO
to incorrectly warn that the game is not using WASAPI. This warning is suppressed for RS2011.

More importantly, RS2011 **does not select its guitar input through normal WASAPI device
enumeration**. Instead, it identifies the Real Tone Cable by its specific WASAPI device path
(a `{flow}.{endpoint-GUID}` string assigned by Wine/Proton), opens it directly in exclusive
mode, and begins polling. Wine/Proton's WASAPI implementation rejects all audio formats offered
by the game for the Real Tone Cable in exclusive mode — making the cable unusable without
RS ASIO.

### The RS ASIO solution for RS2011

RS ASIO intercepts `IMMDevice::Activate` for IAudioClient. When the game activates the Real
Tone Cable's WASAPI device, RS ASIO detects the match (via `WasapiDevice=` in the INI) and
returns an ASIO-backed `RSAsioAudioClient` instead of Wine's broken implementation. The game
receives audio from the configured ASIO input channel transparently.

Audio format negotiation is also handled: RS2011 offers float32 mono as its preferred format,
which is compatible with WineASIO's native `ASIOSTFloat32LSB` type — no conversion needed.

The ASIO host is shared between output (already running for music playback) and input, using
the same buffer size and sample rate. Only sample rate and buffer size are compared when the
second client joins the shared host — format tag differences between output (PCM16) and input
(float32 extensible) are intentionally ignored.
