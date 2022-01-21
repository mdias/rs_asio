# Focusrite Scarlett 18i8 2nd Gen + (optional) ASIO Link Pro

ASIO Link Pro is free these days and is useful to capture ASIO OUT within the OS - for streaming, for example)
When using ASIO Link Pro, **do not** pre-start it using ASIO Link Pro Tool - let the game fail to load it (it will complain several times right after start)
and wait about 10 seconds (till Rocksmith 2014 screen with "press Enter") - driver will start around then and you will get sound.
It also helps if you pre-configure a profile in ASIO Link Pro UI and save it - then you won't have to deal with driver selection popup every time.

## Config file

**RS_ASIO.ini**

```ini
[Config]
EnableWasapiOutputs=0
EnableWasapiInputs=0
EnableAsio=1

[Asio]
; use ["host"] or ["custom" + BufferSizeMode=144] - somehow that's what Rocksmith demands
; from my driver on Windows 11 and setting any other buffer results in no input (only output).
; "driver" setting is useless, Focusrite ASIO panel does not allow to set a custom buffer size like "144"
; and no other buffer size works.
BufferSizeMode=host
CustomBufferSize=144

[Asio.Output]
; switch drivers (uncomment & comment) here and further down if using ASIO Link Pro
;Driver=ASIO Link Pro
Driver=Focusrite USB ASIO
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
;Driver=ASIO Link Pro
Driver=Focusrite USB ASIO
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.1]
;Driver=ASIO Link Pro
Driver=Focusrite USB ASIO
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
