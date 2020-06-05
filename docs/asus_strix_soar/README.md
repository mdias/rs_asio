# Asus Strix Soar

Works without problems using the driver supplied buffer size.
Rocksmith's `LatencyBuffer` can be set to 3 on faster systems.

Instrument can only be heard when using `EnableWasapiInputs=1` so that the RealTone Cable is found and used by the Software.

## Config file

**RS_ASIO.ini**

```ini
[Config]
EnableWasapiOutputs=0
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
Driver=STRIX SOUND CARD ASIO
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=
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

**Rocksmith.ini**
```ini
[Audio]
EnableMicrophone=0
ExclusiveMode=1
LatencyBuffer=3
ForceDefaultPlaybackDevice=
ForceWDM=0
ForceDirectXSink=0
DumpAudioLog=0
MaxOutputBufferSize=0
RealToneCableOnly=0
Win32UltraLowLatencyMode=1
```
