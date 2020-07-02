# Audient Evo v4

## About

This device is confirmed to be working as of _v0.5.2_

### Known issues

 - Guitar is only coming through when using (and configuring for) input 2. 

## Setup steps

### Install drivers

Install the latest [official driver](https://audient.com/products/audio-interfaces/sono/downloads/) (v4.0.9 as of time of writing).

### Set asio control panel settings

Use a DAW such as [Reaper](https://www.reaper.fm/) to open the asio control panel for the Evo 4.

![image](https://user-images.githubusercontent.com/10779424/86245120-ef2bad80-bba0-11ea-96cb-c04e88f91391.png)

There isn't actually any panel but you should see an `e` logo in your taskbar. Right click it and set the sample rate to 48KHz. 

> Note: if you don't do this, you will see an error when Rocksmith opens stating there is no output device detected.

### Setup RS_ASIO

Follow the [configuration guide](https://github.com/mdias/rs_asio#basic-configuration-guide) and set desired outputs and inputs to `Audient USB Audio ASIO Driver`.

If you're using the interface for input, **only the second input seems to be working**. Not too sure why this is but you'll need to set the channel of your input to `1` and use the second TRS input.


## Config files


<details>
<summary>My RS_ASIO.ini</summary>



```
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
Driver=Audient USB Audio ASIO Driver
BaseChannel=0
EnableSoftwareEndpointVolumeControl=0
EnableSoftwareMasterVolumeControl=0
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=Audient USB Audio ASIO Driver
Channel=1
EnableSoftwareEndpointVolumeControl=0
EnableSoftwareMasterVolumeControl=0
SoftwareMasterVolumePercent=100

[Asio.Input.1]
Driver=
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
```

</details>

<details>
<summary>My Rocksmith.ini</summary>



```
[Audio]
EnableMicrophone=1
ExclusiveMode=1
LatencyBuffer=2
ForceDefaultPlaybackDevice=
ForceWDM=0
ForceDirectXSink=0
DumpAudioLog=0
MaxOutputBufferSize=
RealToneCableOnly=0
Win32UltraLowLatencyMode=1
[Renderer.Win32]
ShowGamepadUI=0
ScreenWidth=2560
ScreenHeight=1440
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
DisableBrowser=0
[Net]
UseProxy=1
```

</details>
