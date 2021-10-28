# iRig Pro Duo I/O

known working config file for IK Multimedia iRig Pro Duo I/O - https://www.ikmultimedia.com/products/irigproduoio/
Using "iRig USB ASIO Driver for Windows (v.5.22.0)" ASIO drivers from https://www.ikmultimedia.com/userarea/drivers/
Tested as of October 28, 2021

Input 1 on the interface is ASIO input 0, input 2 on device is ASIO input 1, remove the ";" from the driver line of the "[Asio.Input.1]" block to enable multiplayer

The below "Actual" measured latency figures were recorded by connecting the headphone output to the instrument 1 input and using this utility - https://oblique-audio.com/rtl-utility.php

Custom buffer values being used here as you can leave the driver set to something more suitable for other software and just force ROcksmith friendly settings on launch. 
16 is too aggressive for Rocksmith in my testing. - Driver reports this as 0.3ms, RTL measures it at 10.1 ms total latency.

32 works but has some minor crackling on my test system, might be ok on yours. - Driver reports this as 0.7ms, RTL measures it at 10.4 ms total latency.

64 works perfectly on my system and should be a safe starting point. - Driver reports this as 1.3ms, RTL measures it at 11.1 ms total latency.
128 works well and should be fine even on older systems. - Driver reports this as 2.7ms, RTL measures it at 14.4 ms total latency.

## Config file

**RS_ASIO.ini**

```ini
[Config]
EnableWasapiOutputs=0
EnableWasapiInputs=0
EnableAsio=1

[Asio]
; available buffer size modes:
;    driver - respect buffer size setting set in the driver
;    host   - use a buffer size as close as possible as that requested by the host application
;    custom - use the buffer size specified in CustomBufferSize field
BufferSizeMode=custom
CustomBufferSize=64

[Asio.Output]
Driver=iRig Device
BaseChannel=0
AltBaseChannel=
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=iRig Device
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.1]
;Driver=iRig Device
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.Mic]
Driver=
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
```
