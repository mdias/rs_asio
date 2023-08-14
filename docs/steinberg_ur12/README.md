# Steinberg UR12, UR22, UR22mkII

## Config files - Guitar plugged into 1/4in (input 2) jack on the interface

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
BufferSizeMode=driver
CustomBufferSize=

[Asio.Output]
Driver=Yamaha Steinberg USB ASIO
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
;Driver=Yamaha Steinberg USB ASIO
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.1]
Driver=Yamaha Steinberg USB ASIO
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
```

## Config file - If plugging in with XLR (input 1)

**RS_ASIO.ini**

```ini
[Config]
EnableWasapi=0
EnableAsio=1

[Asio]
; available buffer size modes:
;    driver - respect buffer size setting set in the driver
;    host   - use a buffer size as close as possible as that requested by the host application
;    custom - use the buffer size specified in CustomBufferSize field
BufferSizeMode=driver
CustomBufferSize=

[Asio.Output]
Driver=Yamaha Steinberg USB ASIO
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=Yamaha Steinberg USB ASIO
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.1]
;Driver=Yamaha Steinberg USB ASIO
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
```