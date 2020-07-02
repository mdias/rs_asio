# RS ASIO

This project aims to add ASIO support to `Rocksmith 2014 Edition - Remastered` in order to avoid issues with some WASAPI drivers.
It patches game code at runtime to allow intervening in the process of WASAPI device enumeration so that we can inject our own fake WASAPI devices which internally use ASIO audio API.

## How to use

- Copy the contents (`avrt.dll`, `RS_ASIO.dll`, `RS_ASIO.ini`) of [latest release](https://github.com/mdias/rs_asio/releases/latest) (zip arhive release-xxx.zip) to the game folder.
  - Only the Steam version of Rocksmith is currently supported. You can find local folder of a game by right clicking on a Rocksmith in your Steam library, and selecting menu "Manage" -> "Browse local files"
- Modify the RS_ASIO.ini file to configure which ASIO driver to use, and which channels etc...
- Look into [basic configuration guide](#basic-configuration-guide)
- Make sure Rocksmith.ini is set to run with `ExclusiveMode=1` and `Win32UltraLowLatencyMode=1`. If in doubt, use default settings.
- Make sure your interface clock is set to 48kHz. RS ASIO will try to request 48kHz mode, but your drivers may or may not allow this, so it might help setting it manually.
- Extra: An RS_ASIO-log.txt file is generated inside the game directory which may help discover your ASIO driver name and diagnose issues.
- Look into [list of knows issues](#known-issues) if you experience any problems

### How to remove/uninstall

- Remove the custom DLL files from the game folder.

## Audio Interfaces reported to work well

- [Asus Strix Soar](docs/asus_strix_soar/README.md)
- [Audient Evo 4](docs/audient_evo_4/README.md)
- Audient iD4
- [Behringer MIC2 USB](docs/behringer_mic2usb/README.md), using ASIO4All
- Behringer U-Phoria UM2 [(see this for more details)](https://github.com/mdias/rs_asio/issues/7)
- Behringer UMC1820
- Behringer UMC202HD
- Behringer UMC204HD
- [Behringer UMC404HD](https://github.com/mdias/rs_asio/issues/13)
- [Behringer XR18](https://github.com/mdias/rs_asio/issues/72)
- ESI MAYA22 USB
- [Focusrite Clarett 4Pre USB](https://github.com/mdias/rs_asio/issues/42)
- Focusrite Saffire Pro 40
- Focusrite Scarlett 2i2 1st Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 2i2 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 2i2 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 2i4 1st Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 2i4 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 4i4 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 6i6 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 18i20 2nd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 2nd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 3rd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- Juli@ XTe
- Lexicon Alpha
- Line6 HX Stomp
- M-Audio 2x2
- M-Audio M-Track Plus II
- [M-Audio MobilePre mkII](https://github.com/mdias/rs_asio/issues/15)
- Mackie Onyx Producer 2x2
- Native Instruments Komplete Audio 6
- MOTU 2408 mk3
- MOTU M4
- MOTU UltraLite AVB
- Presonus Quantum 2
- Presonus Studio 24c
- RME Babyface PRO
- RME Multiface with HDSPe PCIe
- Roland Rubix 44
- [Roland ua55](docs/roland_ua_55/README.md)
- [Steinberg UR22mkII](docs/steinberg_ur12/README.md)
- Universal Audio Apollo Twin USB
- [XTONE Smart Stomp](docs/xtone_smartstomp/README.md), using ASIO4All
- Zoom R24
- Zoom U-44

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
