# BOSS Katana MkII

**RS_ASIO.ini**

```ini
[Config]
; use WasapiOutput if you want to use the Katana's speaker by feeding in the audio into the aux-in
; not needed when using headphones (or external speakers) via the rec/headphone out
; To fix error where Rocksmith gets no signal try to turn the Katana On and Off, and make sure that the guitar is pluged in before you turn it on
EnableWasapiOutputs=0
EnableWasapiInputs=0
EnableAsio=1

[Asio]
; available buffer size modes:
;    driver - respect buffer size setting set in the driver
;    host   - use a buffer size as close as possible as that requested by the host application
;    custom - use the buffer size specified in CustomBufferSize field
; use the Katana's driver control panel to configure the buffer size/samplerate (Startmenu → Boss → KATANA)
BufferSizeMode=driver
CustomBufferSize=

; output audio via the Katana. NOTE: Audio fed into the Katana using the primary USB-channel is not output
; via the speaker, so you need to use headphones/plug your speaker into the Katana's rec/headphone out
; if you want to use the Katana's speaker, you'd need to use a multi-device setup and feed the audio into
; the aux-in of the Katana
; if you want to hear Rocksmith through your PC speakers change Driver to Driver=  (yes, empty) and set EnableWasapiOutputs=1 under Config at the top.
; If you want to hear yourself through your speakers (using Rocksmith AMP) put Katana on stand-by mode
; and if you want to hear yourself through your AMP using your AMP settings go to rocksmith mixer in-game and turn guitar 1 volume to 0 (so you won't hear it twice).
[Asio.Output]
Driver=KATANA
BaseChannel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100

; use the Katana's DI signal output via the SECONDARY input
; default singal level might be too low for Rocksmith calibration to pass,
; but in Boss Tone Studio to bump the "dry out level" to 200% if necessary
; (Boss Tone Studio → System → USB-Settings)
; or use the SoftwareMasterVolumePercent control below
;  * to use tones by Rocksmith: turn channel volume to zero
;  * to use Katana's tones: use the mixer in Rocksmith to turn the guitar down
[Asio.Input.0]
Driver=KATANA
Channel=2
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100


[Asio.Input.1]
Driver=
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
