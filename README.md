# RS ASIO

This project aims to add ASIO support to `Rocksmith 2014 Edition - Remastered` in order to avoid issues with some WASAPI drivers.
It patches game code at runtime to allow intervening in the process of WASAPI device enumeration so that we can inject our own fake WASAPI devices which internally use ASIO audio API.

## Other Languages
[简体中文](README_CN.md)

## How to use

- Copy the contents (`avrt.dll`, `RS_ASIO.dll`, `RS_ASIO.ini`) of [latest release](https://github.com/mdias/rs_asio/releases/latest) (zip archive release-xxx.zip) to the game folder.
  - Only the Steam version of Rocksmith is currently supported. You can find local folder of a game by right clicking on a Rocksmith in your Steam library, and selecting menu "Manage" -> "Browse local files"
- Modify the RS_ASIO.ini file to configure which ASIO driver to use, and which channels etc...
- Look into [basic configuration guide](#basic-configuration-guide)
- Make sure Rocksmith.ini is set to run with `ExclusiveMode=1` and `Win32UltraLowLatencyMode=1`. If in doubt, use default settings.
- Make sure your game is set to use the RTC input instead of the microphone one. ([See this](https://github.com/mdias/rs_asio/issues/275#issuecomment-1120386256))
- Make sure your interface clock is set to 48kHz. RS ASIO will try to request 48kHz mode, but your drivers may or may not allow this, so it might help setting it manually.
- Make sure you're NOT using the NoCableLauncher or similar otherwise your instruments may not be detected properly.
- Extra: An RS_ASIO-log.txt file is generated inside the game directory which may help discover your ASIO driver name and diagnose issues.
    - IMPORTANT: Only 32-bit ASIO drivers will be detected!
- Look into [list of knows issues](#known-issues) if you experience any problems

### How to remove/uninstall

- Remove the custom DLL files from the game folder.

### Streaming while using RS ASIO

Check out [this guide](docs/streaming/README.md).

### Using RS ASIO on linux

If you're using Ubuntu 12.04 LTS check [this page](docs/linux/ubuntu_1204_lts.md).

Some people have had success using RS ASIO with [wineasio](https://www.wineasio.org/) on linux. You can check out [this issue](https://github.com/mdias/rs_asio/issues/99) for more information.

## Audio Interfaces reported to work well

- [Alesis Core 1](https://github.com/mdias/rs_asio/issues/115)
- [Antelope Audio Zen Go](https://github.com/mdias/rs_asio/issues/294#issuecomment-2212061137)
- [Antelope Audio Zen Tour](https://github.com/mdias/rs_asio/issues/294)
- [Arturia AudioFuse](https://github.com/mdias/rs_asio/issues/114)
- [Arturia MiniFuse 1](https://github.com/mdias/rs_asio/issues/287)
- [Arturia MiniFuse 2](https://github.com/mdias/rs_asio/issues/282)
- [Asus Strix Soar](docs/asus_strix_soar/README.md)
- [Audient Evo 4](docs/audient_evo_4/README.md)
- [Audient Evo 16](docs/audient_evo_4/README.md)
- [Audient iD14 Mk II](https://github.com/mdias/rs_asio/issues/553)
- [Audient iD4](https://github.com/mdias/rs_asio/issues/295)
- Audient iD22
- [Avid Mbox Studio](https://github.com/mdias/rs_asio/issues/467)
- [Behringer GUITAR 2 USB](https://github.com/mdias/rs_asio/issues/246), using ASIO4All
- [Behringer MIC2 USB](docs/behringer_mic2usb/README.md), using ASIO4All
- Behringer U-Phoria UM2  [(see this for more details)](https://github.com/mdias/rs_asio/issues/7) **IMPORTANT: needs legacy ASIO driver**
- [Behringer UMC22](https://github.com/mdias/rs_asio/issues/326)  **IMPORTANT: needs legacy ASIO driver**
- Behringer UMC1820
- Behringer UMC202HD
- [Behringer UMC204](https://github.com/mdias/rs_asio/issues/156)
- [Behringer UMC204HD](https://github.com/mdias/rs_asio/issues/161)
- [Behringer UMC404HD](https://github.com/mdias/rs_asio/issues/13)
- [Behringer XENIX Q502USB](https://github.com/mdias/rs_asio/issues/132) **IMPORTANT: needs legacy ASIO driver**
- [Behringer XR18](https://github.com/mdias/rs_asio/issues/72)
- [BOSS Gigcaster (GCS-8)](docs/boss_gcs_8/README.md)
- [BOSS GT-1](https://github.com/mdias/rs_asio/issues/494)
- [BOSS Katana-Air](https://github.com/mdias/rs_asio/issues/359)
- [BOSS Katana MkII](docs/katana_mk2/README.md)
- [Creative Live! Audio A3](https://github.com/mdias/rs_asio/issues/412)
- [Creative Sound Blaster Z](https://github.com/mdias/rs_asio/issues/191)
- [Darkglass Element](https://github.com/mdias/rs_asio/issues/258)
- [Devine Centro 2i2o](https://github.com/mdias/rs_asio/issues/513#issuecomment-2733642765)
- ESI MAYA22 USB
- [ESI MAYA44 eX](https://github.com/mdias/rs_asio/issues/134) [(Known issues)](#known-issues)
- [ESI U22 XT](https://github.com/mdias/rs_asio/issues/458)
- [ESI UGM96](https://github.com/mdias/rs_asio/issues/250) [(Known issues)](#known-issues)
- [Fender Link I/O](https://github.com/mdias/rs_asio/issues/565)
- [Fender Mustang Micro Plus](https://github.com/mdias/rs_asio/issues/558#issuecomment-3139856047)
- [Focusrite Clarett 2Pre Thunderbolt](https://github.com/mdias/rs_asio/issues/146)
- [Focusrite Clarett 4Pre USB](https://github.com/mdias/rs_asio/issues/42)
- [Focusrite Clarett 8Pre USB](https://github.com/mdias/rs_asio/issues/158)
- [Focusrite Saffire 6 USB 2.0](https://github.com/mdias/rs_asio/issues/116)
- Focusrite Saffire Pro 40
- [Focusrite Scarlett 2i2 1st Gen](https://github.com/mdias/rs_asio/issues/330) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 2nd Gen](https://github.com/mdias/rs_asio/issues/126) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 3rd Gen](https://github.com/mdias/rs_asio/issues/208) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 4th Gen](https://github.com/mdias/rs_asio/issues/405#issuecomment-1876314996) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i4 1st Gen](https://github.com/mdias/rs_asio/issues/133) [(Known issues)](#known-issues)
- Focusrite Scarlett 2i4 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 4i4 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 6i6 2nd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett 8i6 3rd Gen](https://github.com/mdias/rs_asio/issues/208) [(Known issues)](#known-issues)
- [Focusrite Scarlett 16i16 4th Gen](https://github.com/mdias/rs_asio/issues/405#issuecomment-2527560680) [(Known issues)](#known-issues)
- [Focusrite Scarlett 18i8 2nd Gen](docs/focusrite_scarlett_18i8_2nd_gen_OR_asio_link_pro/README.md) [(Known issues)](#known-issues)
- Focusrite Scarlett 18i8 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 18i20 2nd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 2nd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 3rd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [Fractal Audio AXE FX III](https://github.com/mdias/rs_asio/issues/347)
- [Hotone Ampero II Stage](https://github.com/mdias/rs_asio/issues/444)
- [Hotone Ampero II Stomp](https://github.com/mdias/rs_asio/issues/444)
- [Hotone Jogg](https://github.com/mdias/rs_asio/issues/533)
- [IK Multimedia AXE I/O](https://github.com/mdias/rs_asio/issues/147)
- [IK Multimedia AXE I/O ONE](https://github.com/mdias/rs_asio/issues/490)
- [IK Multimedia iRig](https://github.com/mdias/rs_asio/issues/164), using ASIO4ALL
- [IK Multimedia iRig HD 2](https://github.com/mdias/rs_asio/issues/117)
- [IK Multimedia iRig HD X](https://github.com/mdias/rs_asio/issues/435)
- [IK Multimedia iRig Pro Duo I/O](docs/irig_pro_duo_io/README.md)
- [JackRouter ASIO driver](https://github.com/mdias/rs_asio/issues/303)
- Juli@ XTe
- Lexicon Alpha
- [Line6 AMPLIFi 75](https://github.com/mdias/rs_asio/issues/97) **Some limitations apply. Follow the link for more information.**
- [Line6 HX Stomp](https://github.com/mdias/rs_asio/issues/251)
- [Line6 POD Go](https://github.com/mdias/rs_asio/pull/171)
- M-Audio 2x2
- [M-Audio AIR 192|4](https://github.com/mdias/rs_asio/issues/98)
- [M-Audio Fast Track 2](https://github.com/mdias/rs_asio/issues/175)
- [M-Audio Fast Track Pro USB](https://github.com/mdias/rs_asio/issues/225) (reporter had clock issues, check link)
- [M-Audio Fast Track Ultra 8R](https://github.com/mdias/rs_asio/issues/135)
- M-Audio M-Track Plus II
- [M-Audio M-Track Solo](https://github.com/mdias/rs_asio/issues/207) NOTE: [there is a report](https://github.com/mdias/rs_asio/issues/562) that the 32-bit drivers are crashing
- [M-Audio M-Track Duo](https://github.com/mdias/rs_asio/issues/207)
- [M-Audio MobilePre mkII](https://github.com/mdias/rs_asio/issues/15)
- [M-Audio ProFire 2626](https://github.com/mdias/rs_asio/issues/212#issuecomment-917706302)
- [Mackie Big Knob Studio+](https://github.com/mdias/rs_asio/issues/385)
- [Mackie Onyx 1620i](https://github.com/mdias/rs_asio/issues/239)
- [Mackie Onyx Artist 1x2](https://github.com/mdias/rs_asio/issues/211)
- [Mackie Onyx Producer 2x2](https://github.com/mdias/rs_asio/issues/211)
- [MIDIPLUS Studio S](docs/midiplus_studio_s/README.md)
- [Miditech AUDIOLINK III](https://github.com/mdias/rs_asio/issues/512), using ASIO4ALL
- [Mooer Steep I](https://github.com/mdias/rs_asio/issues/524), using WineASIO (Steam Deck + Proton 9.0)
- [Monoprice Stage Right STi12](https://github.com/mdias/rs_asio/issues/357)
- MOTU 2408 mk3
- [MOTU M2](https://github.com/mdias/rs_asio/issues/151)
- [MOTU M4](https://github.com/mdias/rs_asio/issues/240)
- MOTU UltraLite AVB
- [MOTU Ultralite mk4](https://github.com/mdias/rs_asio/issues/95)
- [Native Instruments Audio Kontrol 1](https://github.com/mdias/rs_asio/issues/131)
- [Native Instruments Komplete Audio 1](https://github.com/mdias/rs_asio/issues/118)
- [Native Instruments Komplete Audio 2](https://github.com/mdias/rs_asio/issues/120)
- Native Instruments Komplete Audio 6
- [Native Instruments Rig Kontrol 2](https://github.com/mdias/rs_asio/issues/214)
- [Native Instruments Rig Kontrol 3](https://github.com/mdias/rs_asio/issues/536)
- [Neural DSP Nano Cortex](https://github.com/mdias/rs_asio/issues/529)
- [Neural DSP Quad Cortex](https://github.com/mdias/rs_asio/issues/294)
- [NUX Mighty Air](https://github.com/mdias/rs_asio/issues/364)
- [NUX Mighty Lite BT MKII](https://github.com/mdias/rs_asio/issues/446)
- [NUX Mighty Plug](https://github.com/mdias/rs_asio/issues/117)
- [PositiveGrid Spark 40](https://github.com/mdias/rs_asio/issues/218) (issue reported with limited sample rate support in 2024/07; see linked issue)
- [Presonus AudioBox iTwo](https://github.com/mdias/rs_asio/issues/140)
- [Presonus AudioBox USB 96](https://github.com/mdias/rs_asio/issues/140)
- [Presonus ioStation 24c](https://github.com/mdias/rs_asio/issues/542)
- Presonus Quantum 2
- [Presonus Quantum ES 4](https://github.com/mdias/rs_asio/issues/496)
- [Presonus Studio 24c](https://github.com/mdias/rs_asio/issues/280)
- [Presonus StudioLive AR12c](https://github.com/mdias/rs_asio/issues/196)
- ReaRoute ASIO (Reaper virtual ASIO router)
- RME Babyface PRO
- RME HDSPe AIO
- RME Multiface with HDSPe PCIe
- Roland Rubix 22
- Roland Rubix 44
- [Roland UA-1G](https://github.com/mdias/rs_asio/issues/291#issuecomment-1179084242)
- [Roland ua55](docs/roland_ua_55/README.md)
- [Roland V-Studio 100](https://github.com/mdias/rs_asio/issues/91)
- [Røde AI-1](https://github.com/mdias/rs_asio/issues/339)
- [Solid State Logic SSL12](https://github.com/mdias/rs_asio/issues/167)
- [Solid State Logic SSL2+](https://github.com/mdias/rs_asio/issues/167)
- Sonicake Sonic Cube(Use channel 1 for input)
- [SoundCraft Notepad-8FX](https://github.com/mdias/rs_asio/issues/86)
- [SoundCraft Notepad-12FX](https://github.com/mdias/rs_asio/issues/86)
- [SoundCraft Ui24R](https://github.com/mdias/rs_asio/issues/528)
- Sound Devices MixPre-6 II
- [Steinberg CI1](https://github.com/mdias/rs_asio/issues/268)
- [Steinberg IXO12](https://github.com/mdias/rs_asio/issues/495)
- [Steinberg MR816 CSX](https://github.com/mdias/rs_asio/issues/448)
- [Steinberg UR12](docs/steinberg_ur12/README.md)
- [Steinberg UR22](docs/steinberg_ur12/README.md)
- [Steinberg UR22mkII](docs/steinberg_ur12/README.md)
- [Steinberg UR22C](https://github.com/mdias/rs_asio/issues/124)
- [Steinberg UR24C](https://github.com/mdias/rs_asio/issues/341)
- [Steinberg UR44C](https://github.com/mdias/rs_asio/issues/130)
- [Sterling Harmony H224](https://github.com/mdias/rs_asio/issues/390)
- [SuZhou UTECK's Guitar-Cube Chord A](https://github.com/mdias/rs_asio/issues/92)
- Swissonic UA-2x2
- [TASCAM US-1x2](https://github.com/mdias/rs_asio/issues/266)
- [TASCAM US-4x4HR](https://github.com/mdias/rs_asio/issues/402)
- [TC HELICON Blender](https://github.com/mdias/rs_asio/issues/407)
- [Teyun Q22](https://github.com/mdias/rs_asio/issues/520), using ASIO4All
- [Teyun Q24](https://github.com/mdias/rs_asio/issues/427), using ASIO4All
- [Universal Audio Apollo Twin USB](https://github.com/mdias/rs_asio/issues/307) (requires older driver)
- [Universal Audio Volt 1](docs/universal_audio_volt1/README.md)
- [Universal Audio Volt 2 USB](https://github.com/mdias/rs_asio/issues/462)
- [Universal Audio Volt 276](docs/universal_audio_volt_276/README.md)
- [XTONE Smart Stomp](docs/xtone_smartstomp/README.md), using ASIO4All
- [Yamaha AG06 USB Mixing Console](https://github.com/mdias/rs_asio/issues/81)
- [Yamaha THR10II](https://github.com/mdias/rs_asio/issues/210)
- [Yamaha THR30IIW](https://github.com/mdias/rs_asio/issues/210)
- [Zoom AMS-22](https://github.com/mdias/rs_asio/issues/518)
- [Zoom G2.1NU](https://github.com/mdias/rs_asio/issues/400)
- [Zoom G2.1DM](https://github.com/mdias/rs_asio/issues/400)
- Zoom H2N
- [Zoom H6](https://github.com/mdias/rs_asio/issues/198)
- Zoom R24
- [Zoom U-22](https://github.com/mdias/rs_asio/issues/179)
- [Zoom U-44](https://github.com/mdias/rs_asio/issues/334)
- Zoom UAC-2
- [ZOOM UAC-232](https://github.com/mdias/rs_asio/issues/431)

### Basic configuration guide

1. Follow installation steps, described [above](#how-to-use)
1. Run Rocksmith for the first time.
1. Look into `RS_ASIO-log.txt`, you will see names of drivers

```txt
0.456 [INFO]  AsioHelpers::FindDrivers
0.456 [INFO]    ASIO4ALL v2
0.457 [INFO]    MOOER USB Audio
0.457 [INFO]    XMOS USB Audio 2.0 ST 3086
0.457 [INFO]    ZOOM R16_R24 ASIO Driver
```

4. Copy name of the corresponding driver for the given device (such as `MOOER USB Audio` or `Focusrite USB ASIO`) to the `Driver` property of the `[Asio.Output]`, `[Asio.Input.0]`, `[Asio.Input.1]` and `[Asio.Input.Mic]` blocks of the `RS_ASIO.ini`
1. Run Rocksmith again
1. Repeat until there is no cracks in audio. Your goal is to have smallest possible values without cracks. Find smallest possible LatencyBuffer and then gradually set buffer size until there is no cracks.
    1. Modify LatencyBuffer (try values 4,3,2,1)
    1. Modify buffersize either in ASIO driver control panel or in `CustomBufferSize` option in the `RS_ASIO.ini` file. For the beginning follow rule of thumb that buffer size should be divisible to 32
    1. Run Rocksmith
    1. Look into `RS_ASIO-log.txt` if you experience any issues

### Known issues

- Your interface MUST support 48kHz playback
- Doesn't provide a way to open the ASIO control panel (please configure your interface elsewhere for now, if needed).
- Will need a game reboot if ASIO settings are changed while the game is running (such as changing sample rate, sample type etc).
- Some Focusrite devices have been reported to only output sound properly when using ASIO buffer sizes of 48, 96 or 192. When `BufferSizeMode` is set to `driver` in `RS_ASIO.ini`'s `[Asio]` block, this only has to be set in the Focusrite Settings. When `BufferSizeMode` is set to `custom`, set `CustomBufferSize` to the same value.
    - Changing your windows audio settings to use `2-channel, 24 bit, 48000 Hz (Studio Quality)` format [seems to help with achieving lower buffer sizes](https://github.com/mdias/rs_asio/issues/411).
- [According to reports](https://github.com/mdias/rs_asio/issues?q=label%3A%22focusrite+asio+driver%22+), newer Focusrite driver releases (after 4.102.4) no longer include 32 bit ASIO drivers required by RS ASIO. You can work around this by using software like voicemeeter to reroute audio to the normal 64 bit drivers.
- Some ESI ASIO drivers appear to get stuck when quitting Rocksmith, requiring unplugging the USB and plugging it again to be playable again.
- Hardware hotplugging while the game is running won't be noticed by the game.
- Game sometimes crash on exit with ASIO4ALL

## Donating

If you wish to donate to the developer of this project, you can do so through [paypal.me/mdiasdonations](https://paypal.me/mdiasdonations). However opening an issue to say "thanks" would be enough.
