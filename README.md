# RS ASIO

This project aims to add ASIO support to `Rocksmith 2014 Edition - Remastered` in order to avoid issues with some WASAPI drivers.
It patches game code at runtime to allow intervening in the process of WASAPI device enumeration so that we can inject our own fake WASAPI devices which internally use ASIO audio API.

## How to use

- Copy the contents (`avrt.dll`, `RS_ASIO.dll`, `RS_ASIO.ini`) of [latest release](https://github.com/mdias/rs_asio/releases/latest) (zip archive release-xxx.zip) to the game folder.
  - Only the Steam version of Rocksmith is currently supported. You can find local folder of a game by right clicking on a Rocksmith in your Steam library, and selecting menu "Manage" -> "Browse local files"
- Modify the RS_ASIO.ini file to configure which ASIO driver to use, and which channels etc...
- Look into [basic configuration guide](#basic-configuration-guide)
- Make sure Rocksmith.ini is set to run with `ExclusiveMode=1` and `Win32UltraLowLatencyMode=1`. If in doubt, use default settings.
- Make sure your interface clock is set to 48kHz. RS ASIO will try to request 48kHz mode, but your drivers may or may not allow this, so it might help setting it manually.
- Extra: An RS_ASIO-log.txt file is generated inside the game directory which may help discover your ASIO driver name and diagnose issues.
- Look into [list of knows issues](#known-issues) if you experience any problems

### How to remove/uninstall

- Remove the custom DLL files from the game folder.

### Streaming while using RS ASIO

Check out [this guide](docs/streaming/README.md).

### Using RS ASIO on linux

Some people have had success using RS ASIO with [wineasio](https://www.wineasio.org/) on linux. You can check out [this issue](https://github.com/mdias/rs_asio/issues/99) for more information.

## Audio Interfaces reported to work well

- [Alesis Core 1](https://github.com/mdias/rs_asio/issues/115)
- [Arturia AudioFuse](https://github.com/mdias/rs_asio/issues/114)
- [Asus Strix Soar](docs/asus_strix_soar/README.md)
- [Audient Evo 4](docs/audient_evo_4/README.md)
- Audient iD4
- [Behringer MIC2 USB](docs/behringer_mic2usb/README.md), using ASIO4All
- Behringer U-Phoria UM2  [(see this for more details)](https://github.com/mdias/rs_asio/issues/7) **IMPORTANT: needs legacy ASIO driver**
- Behringer UMC1820
- Behringer UMC202HD
- [Behringer UMC204](https://github.com/mdias/rs_asio/issues/156)
- [Behringer UMC204HD](https://github.com/mdias/rs_asio/issues/161)
- [Behringer UMC404HD](https://github.com/mdias/rs_asio/issues/13)
- [Behringer XENIX Q502USB](https://github.com/mdias/rs_asio/issues/132)
- [Behringer XR18](https://github.com/mdias/rs_asio/issues/72)
- BOSS GT-1
- [BOSS Katana MkII](docs/katana_mk2/README.md)
- ESI MAYA22 USB
- [ESI MAYA44 eX](https://github.com/mdias/rs_asio/issues/134)
- [Focusrite Clarett 2Pre Thunderbolt](https://github.com/mdias/rs_asio/issues/146)
- [Focusrite Clarett 4Pre USB](https://github.com/mdias/rs_asio/issues/42)
- [Focusrite Clarett 8Pre USB](https://github.com/mdias/rs_asio/issues/158)
- [Focusrite Saffire 6 USB 2.0](https://github.com/mdias/rs_asio/issues/116)
- Focusrite Saffire Pro 40
- Focusrite Scarlett 2i2 1st Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 2nd Gen](https://github.com/mdias/rs_asio/issues/126) [(Known issues)](#known-issues)
- Focusrite Scarlett 2i2 3rd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i4 1st Gen](https://github.com/mdias/rs_asio/issues/133) [(Known issues)](#known-issues)
- Focusrite Scarlett 2i4 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 4i4 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 6i6 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 18i20 2nd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 2nd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 3rd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [IK Multimedia AXE I/O](https://github.com/mdias/rs_asio/issues/147)
- [IK Multimedia iRig](https://github.com/mdias/rs_asio/issues/164), using ASIO4ALL
- [IK Multimedia iRig HD 2](https://github.com/mdias/rs_asio/issues/117)
- Juli@ XTe
- Lexicon Alpha
- [Line6 AMPLIFi 75](https://github.com/mdias/rs_asio/issues/97) **Some limitations apply. Follow the link for more information.**
- Line6 HX Stomp
- [Line6 POD Go](https://github.com/mdias/rs_asio/pull/171)
- M-Audio 2x2
- [M-Audio AIR 192|4](https://github.com/mdias/rs_asio/issues/98)
- [M-Audio Fast Track 2](https://github.com/mdias/rs_asio/issues/175)
- [M-Audio Fast Track Ultra 8R](https://github.com/mdias/rs_asio/issues/135)
- M-Audio M-Track Plus II
- [M-Audio MobilePre mkII](https://github.com/mdias/rs_asio/issues/15)
- Mackie Onyx Producer 2x2
- [MIDIPLUS Studio S](docs/midiplus_studio_s/README.md)
- MOTU 2408 mk3
- [MOTU M2](https://github.com/mdias/rs_asio/issues/151)
- MOTU M4
- MOTU UltraLite AVB
- [MOTU Ultralite mk4](https://github.com/mdias/rs_asio/issues/95)
- [Native Instruments Audio Kontrol 1](https://github.com/mdias/rs_asio/issues/131)
- [Native Instruments Komplete Audio 1](https://github.com/mdias/rs_asio/issues/118)
- [Native Instruments Komplete Audio 2](https://github.com/mdias/rs_asio/issues/120)
- Native Instruments Komplete Audio 6
- [NUX Mighty Plug](https://github.com/mdias/rs_asio/issues/117)
- [Presonus AudioBox iTwo](https://github.com/mdias/rs_asio/issues/140)
- Presonus Quantum 2
- Presonus Studio 24c
- RME Babyface PRO
- RME Multiface with HDSPe PCIe
- Roland Rubix 22
- Roland Rubix 44
- [Roland ua55](docs/roland_ua_55/README.md)
- [Roland V-Studio 100](https://github.com/mdias/rs_asio/issues/91)
- [Solid State Logic SSL2+](https://github.com/mdias/rs_asio/issues/167)
- [SoundCraft Notepad-12FX](https://github.com/mdias/rs_asio/issues/86)
- [Steinberg UR22C](https://github.com/mdias/rs_asio/issues/124)
- [Steinberg UR44C](https://github.com/mdias/rs_asio/issues/130)
- [Steinberg UR22mkII](docs/steinberg_ur12/README.md)
- [SuZhou UTECK's Guitar-Cube Chord A](https://github.com/mdias/rs_asio/issues/92)
- Universal Audio Apollo Twin USB
- [XTONE Smart Stomp](docs/xtone_smartstomp/README.md), using ASIO4All
- [Yamaha AG06 USB Mixing Console](https://github.com/mdias/rs_asio/issues/81)
- Zoom R24
- [Zoom U-22](https://github.com/mdias/rs_asio/issues/179)
- Zoom U-44
- Zoom UAC-2

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

4. Copy name of the corresponding driver to the [Asio...] block of the RS_ASIO.ini
1. Run Rocksmith again
1. Repeat until there is no cracks in audio. Your goal is to have smallest possible values without cracks. Find smallest possible LatencyBuffer and then gradually set buffer size until there is no cracks.
    1. Modify LatencyBuffer (try values 4,3,2,1)
    1. Modify buffersize either in ASIO driver control panel or in CustomBufferSize option in the RS_ASIO.ini file. For the beginning follow rule of thumb that buffer size should be divisible to 32
    1. Run Rocksmith
    1. Look into `RS_ASIO-log.txt` if you experience any issues

### Known issues

- Your interface MUST support 48kHz playback
- Doesn't provide a way to open the ASIO control panel (please configure your interface elsewhere for now, if needed).
- Will need a game reboot if ASIO settings are changed while the game is running (such as changing sample rate, sample type etc).
- Some Focusrite devices have been reported to only output sound properly when using ASIO buffer sizes of 48, 96 or 192. You can use the custom buffer size setting on RS_ASIO.ini for this.
- Hardware hotplugging while the game is running won't be noticed by the game.
- Game sometimes crash on exit with ASIO4ALL
