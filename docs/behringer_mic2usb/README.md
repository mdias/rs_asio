# Behringer MIC2 USB

Very nice sound qulity with combination of Digitech BP355 bass processor.  

Please double-check your ini-files settings before reporting an issue.  

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
EnableMicrophone=1
ExclusiveMode=1
LatencyBuffer=1
ForceDefaultPlaybackDevice=Stereo Headset 1
ForceWDM=0
ForceDirectXSink=0
DumpAudioLog=0
MaxOutputBufferSize=0
RealToneCableOnly=0
Win32UltraLowLatencyMode=1
[Renderer.Win32]
ShowGamepadUI=0
ScreenWidth=1600
ScreenHeight=1024
Fullscreen=0
VisualQuality=2
RenderingWidth=0
RenderingHeight=0
EnablePostEffects=1
EnableShadows=1
EnableHighResScope=1
EnableDepthOfField=1
EnablePerPixelLighting=1
MsaaSamples=4
DisableBrowser=1
[Net]
UseProxy=0
```

## Troubleshooting

If the above config results in distorted audio, try changing the `CustomBufferSize` to 192 also in ASIO4ALL control panel.
