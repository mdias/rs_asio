# RS ASIO

This project aims to add ASIO support to Rocksmith 2014 in order to avoid issues with some WASAPI drivers.
It patches game code at runtime to allow intervening in the process of WASAPI device enumeration so that we can inject our own fake WASAPI devices which internally use ASIO audio API.

### Download

You can download the [latest release here](https://github.com/mdias/rs_asio/releases/latest).

### How to use

- Copy the DLL files to the game folder.
- Modify the RS_ASIO.ini file to configure which ASIO driver to use, and which channels etc...
- Make sure your ASIO interface is set to 24-bit mode.
- Make sure Rocksmith.ini is set to run with `ExclusiveMode=1` and `Win32UltraLowLatencyMode=1`. If in doubt, use default settings.
- Extra: An RS_ASIO-log.txt file is generated inside the game directory which may help discover your ASIO driver name and diagnose issues.

### How to remove/uninstall

- Remove the custom DLL files from the game folder.

### Audio Interfaces reported to work well

- Audient iD4
- Behringer U-Phoria UM2 [(see this for more details)](https://github.com/mdias/rs_asio/issues/7)
- Behringer UMC204HD
- Focusrite Scarlett 2i2 2nd Gen (see Known issues)
- Focusrite Scarlett 2i2 3rd Gen (see Known issues)
- Focusrite Scarlett 2i4 1st Gen (see Known issues)
- Focusrite Scarlett 2i4 2nd Gen (see Known issues)
- Focusrite Scarlett Solo 2nd Gen (see Known issues)
- M-Audio 2x2
- M-Audio M-Track Plus II
- Mackie Onyx Producer 2x2
- MOTU UltraLite AVB
- Presonus Quantum 2

### Known issues

- Supports only 32-bit aligned, 24-bit audio sample types as of now.
- Doesn't provide a way to open the ASIO control panel (please configure your interface elsewhere for now, if needed).
- Will need a game reboot if ASIO settings are changed while the game is running (such as changing sample rate, sample type etc).
- Some Focusrite devices have been reported to only output sound properly when using ASIO buffer sizes of 48, 96 or 192. You can use the custom buffer size setting on RS_ASIO.ini for this.
- Hardware hotplugging while the game is running won't be noticed by the game.