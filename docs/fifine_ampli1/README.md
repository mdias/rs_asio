# Fifine Ampli1 (ASIO4ALL v2)

Quick guide to use RS_ASIO with the Fifine Ampli1 via ASIO4ALL v2.

## Prerequisites

- ASIO4ALL v2 installed
- Interface connected to the PC

## Config file

**RS_ASIO.ini**

```ini
[Config]
EnableWasapiOutputs=0
EnableWasapiInputs=0
EnableAsio=1

[Asio]
BufferSizeMode=driver
CustomBufferSize=

[Asio.Output]
Driver=ASIO4ALL v2
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=0

[Asio.Input.0]
Driver=ASIO4ALL v2
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=0

[Asio.Input.1]
Driver=
[Asio.Input.Mic]
Driver=
```

## Notes

- The input is set to channel 1; adjust `Channel` if your instrument is on a different channel.
- If you need WASAPI output, set `EnableWasapiOutputs=1` and clear `Driver` in `Asio.Output`.
- If you hear crackles, try a different buffer size in the ASIO4ALL panel.
