# RS ASIO

This project aims to add ASIO support to Rocksmith 2014 in order to avoid issues with some WASAPI drivers.
It patches game code at runtime to allow intervening in the process of WASAPI device enumeration so that we can inject our own fake WASAPI devices which internally use ASIO audio API.

### How to use

- Copy the DLL files to the game folder.
- Modify the RS_ASIO.ini file to configure which ASIO driver to use, and which channels etc...
- Make sure your ASIO interface is set to 24-bit mode.
- Extra: An RS_ASIO-log.txt file is generated inside the game directory which may help discover your ASIO driver name diagnose issues.

### How to remove/uninstall

- Remove the custom DLL files from the game folder.

### Known issues

- Supports only 32-bit aligned, 24-bit audio sample types as of now.
- Doesn't provide a way to open the ASIO control panel (please configure your interface elsewhere for now, if needed).
- Ignores volume change requests by the game on the input devices.
- Tested only on one device as of 6th, Sep. 2019.
- Will need a game reboot if ASIO settings are changed while the game is running (such as changing sample rate, sample type etc).
- Hardware hotplugging while the game is running won't be noticed by the game.