# Behringer MIC2 USB

Very nice sound qulity with combination of Digitech BP355 bass processor.

Please double-check your ini-files settings before reporting an issue.

## First steps

Run a game, skip calibration, run in-game tuner.

## Config files

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
CustomBufferSize=192
[Asio.Output]
Driver=ASIO4ALL v2
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=ASIO4ALL v2
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
ExclusiveMode=1
LatencyBuffer=1
ForceWDM=0
Win32UltraLowLatencyMode=1
```

## Troubleshooting

If the above config results in distorted audio, try changing the `CustomBufferSize` to 192 also in ASIO4ALL control panel.
