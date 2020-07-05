# RS ASIO

***Ultra low latency for `Rocksmith 2014 Edition - Remastered`***

RS ASIO lets you use your ASIO capable audio interface in Rocksmith, giving you latency as low as `1ms`. That means dead accurate note detection in most cases, turning Rocksmith into the tool we always wanted it to be. RS ASIO also fixes issues with some WASAPI drivers.

It patches game code at runtime to intervene in the process of WASAPI device enumeration, so that we can inject our own fake devices, which internally use the ASIO audio API. 

If you don't have a USB interface, or would like to know if yours works, see the [interface support
](#interface-support-&-troubleshooting) section.

## How to use

- Copy the contents (`avrt.dll`, `RS_ASIO.dll`, `RS_ASIO.ini`) of the [latest release](https://github.com/mdias/rs_asio/releases/latest) (zip arhive release-xxx.zip) to the game folder
  - Only the Steam version of Rocksmith is currently supported. You can find local folder of a game by right clicking on a Rocksmith in your Steam library, and selecting menu "Manage" -> "Browse local files"

- Configure the `RS_ASIO.ini` and `Rocksmith.ini` files
  - See the [configuration guide](#configuration-guide)

- Re-run the in-game `Calibration` (hit `Space` while in the fullscreen tuner)

- Play!

### Uninstallation

- Remove the custom DLL files from the game folder


### Configuration guide

**Example `RS_ASIO.ini´:

```ini
[Config]
EnableWasapiOutputs=1
EnableWasapiInputs=1
EnableAsio=1

[Asio]
; available buffer size modes:
;    driver - respect buffer size setting set in the driver
;    host   - use a buffer size as close as possible as that requested by the host application
;    custom - use the buffer size specified in CustomBufferSize field
BufferSizeMode=driver
CustomBufferSize=

[Asio.Output]
Driver=ASIO 2.0 - ESI MAYA22USB
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=ASIO 2.0 - ESI MAYA22USB
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.1]
Driver=
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
```
1. Follow the installation steps, described [above](#how-to-use)
1. Run Rocksmith for the first time.
1. Check `RS_ASIO-log.txt`, which has now been created in your game folder, and you will find the names of your ASIO drivers

```txt
0.456 [INFO]  AsioHelpers::FindDrivers
0.456 [INFO]    ASIO4ALL v2
0.457 [INFO]    MOOER USB Audio
0.457 [INFO]    XMOS USB Audio 2.0 ST 3086
0.457 [INFO]    ZOOM R16_R24 ASIO Driver
```

4. Copy name of your desired driver to the [Asio...] block, ´Driver´ line
1. Make sure Rocksmith.ini is set to run with `ExclusiveMode=1` and `Win32UltraLowLatencyMode=1`. If in doubt, use default settings
1. Make sure your interface clock is set to 48kHz. RS ASIO will try to request 48kHz mode, but your drivers may or may not allow this, so it might help setting it manually
1. Run Rocksmith again
1. Open ´Rocksmith.ini´, set the lowest possible ´LatencyBuffer´ and then gradually lower the buffer size until there is no noise. Test until there are no cracks or distortion in the audio. Your goal is to achieve the lowest possible values without introducing noise. 
    1. Modify LatencyBuffer (try values 4,3,2,1)
    1. Modify buffersize either in your ASIO driver control panel or in the ´CustomBufferSize´ option in the RS_ASIO.ini file. In most cases the buffer size should be a multiple of 8, and lower is better. Values between 48 and 192 are commo
    

## Interface Support & Troubleshooting

**Before opening an issue, please read this fully and use the search function. 

**When opening an issue, post the contents of `Rocksmith.ini´, ´RS_ASIO.ini´ and ´RS_ASIO-log.txt´.

These are the interface we know for sure work, reported by users like you. If your interface isn't present, please let us know if it works! Even if an interface isn't listed, it's still likely to work just fine.

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

- **Your interface MUST support 48kHz playback
- There's no dedicated ASIO control panel (please configure your interface directly for now, if necessary)
- The game must to be restarted to apply changes to ASIO settings
- Some Focusrite devices have been reported to only output sound properly when using ASIO buffer sizes of 48, 96 or 192. You can use the custom buffer size setting on RS_ASIO.ini for this.
- Hardware hotplugging while the game is running detected by the game
- Game sometimes crashes on exit when using ASIO4ALL

1. **If you're having audio problems
  
  - Check all the cables, trace the signal path from the guitar to the interface
  - Make sure you haven't muted anything & check volumes
  - Read the ´RS_ASIO-log.txt´ file
  - Check the [releases page](https://github.com/mdias/rs_asio/releases/latest) for new versions & changelogs
  - Reboot your PC
