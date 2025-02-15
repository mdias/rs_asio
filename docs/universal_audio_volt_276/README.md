Steps to get RS_ASIO working on a Universal Audio Volt 276 interface.

This guide should work for all devices in the UA Volt series, but adjust the Input0 channel if you are not using input 0. We will also go over how to download the driver for this device. If you have already installed UA Connect and registered your Volt, skip to step 8.


1. Plug in your UA Volt interface
2. Go to the Universal Audio website, and their [downloads page](https://www.uaudio.com/downloads/ua-connect/)
3. Click on the download for your OS (macOS/Windows)
4. Download the UA Connect software for your OS
5. Install the software
6. Once you get into the software, it should notice your UA Volt device. Register your device by logging into / creating a UA account.
7. After you register, it should ask to install the drivers.
8. Go to your Rocksmith folder
9. Paste the following into your RS_ASIO.ini file.

```ini
# for "EnableWasapiOutputs" you can use -1 to have a message prompting
# to use either WASAPI or ASIO for output every time you boot the game
[Config]
EnableWasapiOutputs=0
EnableWasapiInputs=0
EnableAsio=1

[Asio]
; available buffer size modes:
;    driver - respect buffer size setting set in the driver
;    host   - use a buffer size as close as possible as that requested by the host application
;    custom - use the buffer size specified in CustomBufferSize field
BufferSizeMode=driver
CustomBufferSize=

# if your game hangs or crashes on exit, try setting "EnableRefCountHack" to true.
# when blank or invalid, the value of "EnableRefCountHack" will be interpreted as
# true if RS ASIO detects the usage of Asio4All.
# the same applies for all inputs.
[Asio.Output]
Driver=Universal Audio Volt
BaseChannel=0
AltBaseChannel=
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

[Asio.Input.0]
Driver=Universal Audio Volt
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

[Asio.Input.1]
Driver=Universal Audio Volt
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

;[Asio.Input.Mic]
;Driver=ASIO4ALL v2
;Channel=1
;EnableSoftwareEndpointVolumeControl=1
;EnableSoftwareMasterVolumeControl=1
;SoftwareMasterVolumePercent=100
;EnableRefCountHack=
```

Note: This will default your input to channel 0 (left-most jack), and sends the audio output to the Volt device. If you want to use headphones / speakers that are not connected to the Volt, remove the text in the Driver line of Asio.Output and set EnableWasapiOutputs under Config to 1.

10. Now in your Rocksmith.ini file, make sure you have both ExclusiveMode and Win32UltraLowLatencyMode set to 1 as those are required for RS_ASIO to work properly.
11. Launch Rocksmith

I hope this helps!
