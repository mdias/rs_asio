# Audient Evo 4 / Evo 16

## About

This device is confirmed to be working as of _v0.5.3_

_Note: This is tested for single user/player only!_

### Known issues

 - None as of now with the latest driver and patch updates.

## Setup steps

### Install drivers

Install the latest [official driver](https://audient.com/products/audio-interfaces/sono/downloads/) (v4.1.3 as of time of writing).

Note: Also make sure that your interface is working fine. Easiest way is to check by installing Audacity and check if your guitar tone (clean) is being recorded. If it is recording fine and you don't have issues with the driver move ahead.

### ASIO and Control panel settings

Although at the time of writing this you do no need to change any default settings [sample rate (96Kz), depth (24 bit) and buffer size (1024)] for inputs and outputs in the control panel or from the EVO app in the Window's notification tray. You also won't have to disable any input (a microphone in my case) in the control panel. But make sure that your default input is set your `Mic | Line | Instrument 1`. With the current RS_ASIO patch and Audient drivers everything works fine without tinkering any settings.

Here are the settings on my computer.

![image](https://raw.githubusercontent.com/AmolAmrit/rs_asio/master/docs/audient_evo_4/Annotation%202020-08-16%20224310.png)

![image](https://raw.githubusercontent.com/AmolAmrit/rs_asio/master/docs/audient_evo_4/Annotation%202020-08-16%20224424.png)

![image](https://raw.githubusercontent.com/AmolAmrit/rs_asio/master/docs/audient_evo_4/Annotation%202020-08-16%20232812.png)

![image](https://raw.githubusercontent.com/AmolAmrit/rs_asio/master/docs/audient_evo_4/Annotation%202020-08-16%20232728.png)

_Note:_ **Rocksmith however supports input only at a sample rate till 48Kz. The RS_ASIO patch automatically sets the sample rate to 48Kz so by default you won't need to hinder any settings. But in case if you face issues with it is always recommende to set the sample rate manually to 48Kz from the EVO app.** 

![image](https://user-images.githubusercontent.com/10779424/86245120-ef2bad80-bba0-11ea-96cb-c04e88f91391.png)

There isn't actually any panel but you should see an `e` logo in your taskbar. Right click it and set the sample rate to 48KHz. 

### Setup RS_ASIO

Follow the instructions from the [configuration guide](https://github.com/mdias/rs_asio#basic-configuration-guide) to set the desired outputs and inputs to `Audient USB Audio ASIO Driver`.

In the EVO 4 the arrangement of input is a little different. So don't get confused from the configuration file. The Channel=0 is actually Channel=1 in the EVO 4. I'll be sharing my files below too. You can see that I have commented out Asio Input 1 by using a semi colon. Because if you don't then Rocksmith will call for both inputs and your interface won't be detected as the Rocksmith cable.

_Note: Our Renderer.Win32 might be different depending on our video configuration._


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
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.0]
Driver=Audient USB Audio ASIO Driver
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

[Asio.Input.1]
;Driver=
;Channel=1
;EnableSoftwareEndpointVolumeControl=1
;EnableSoftwareMasterVolumeControl=1
;SoftwareMasterVolumePercent=100
```

</details>

<details>
<summary>My Rocksmith.ini</summary>



```
[Audio]
EnableMicrophone=1
ExclusiveMode=1
LatencyBuffer=4
ForceDefaultPlaybackDevice=1
ForceWDM=0
ForceDirectXSink=0
DumpAudioLog=0
MaxOutputBufferSize=0
RealToneCableOnly=0
Win32UltraLowLatencyMode=1
[Renderer.Win32]
ShowGamepadUI=0
ScreenWidth=1920
ScreenHeight=1080
Fullscreen=2
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

## In the game

_**After you have done all of this setting up. It is very important to calibrate your guitar. If you don't then some strings may not be detected as it happened with me**_

## Credits: 
[For Rocksmith.ini](https://www.reddit.com/r/rocksmith/comments/4qt9fa/solution_for_crackling_noisetoo_much_distortion/)

[For solving issues](https://www.reddit.com/r/rocksmith/comments/i9nbix/help_with_rs_asio/)