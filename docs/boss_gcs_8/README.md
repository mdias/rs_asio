# BOSS Gigcaster 8 (GCS-8)

This is the configuration for using a Boss Gigcaster 8 (GCS-8) 
for audio output and guitar input on Windows.

## Install the Boss GCS-8 Drivers and Switch to MKT-RECORD Mode
To use the ASIO drivers you must switch the Gigcaster to 
the audio mode `MKT-RECORD` so that it uses its own driver
to send individual channels over USB (by default it sends 
the mix and uses the standard Windows driver not the Boss one).

First, install the Windows drivers for the Gigcaster, available
from the Boss product support page.

Now on the Gigcaster, go to the Main Menu, push the three-line 
"hamburger" icon to enter the main Menu, push Setup (gear icon) 
and then the USB icon. Select audio mode `MKT-RECORD`. Unplug and plug 
in the USB cable to make the computer mount the device with the new 
Boss driver.

## Connect the Guitar with Effects Disabled
Connect the guitar to the front jack and turn off 
the effects for the guitar channel to send a clean signal 
(Press the Channel 1/guitar button, then Effects to turn them off - 
the circle icon at the top should be grey not green).

Make sure send the Channel 1 (no light in the dashed loudspeaker icon),
and turn of the monitoring to hear only the processed sound from the 
computer (no light in the headphone icon).

The computer mix will go to the USB channel, so put that in your 
headphone or the main output.

## RS_ASIO.ini Config File

**RS_ASIO.ini**

```ini
;; This is the configuration for using a Boss Gigcaster 8 (GCS-8) 
;; for audio output and guitar input on Windows.
;;
;; To use the ASIO drivers you must switch the Gigcaster to 
;; the audio mode MKT-RECORD so that it uses its own driver
;; to send individual channels over USB (by default it sends 
;; the mix and uses the standard Windows driver not the Boss one).
;; First, install the Windows drivers for the Gigcaster, available
;; from the Boss product support page.
;; Go to Main Menu, push the three-line "hamburger" icon to enter 
;; the main Menu, push Setup (gear icon) and then the USB icon.
;; Select audio mode MKT-RECORD. Unplug and plug in the USB cable
;; to make the computer mount the device with the new Boss driver.
;;
;; Connect the guitar to the front jack and turn off 
;; the effects for the guitar channel to send a clean signal 
;; (Press the Channel 1/guitar button, then Effects to turn them off
;; - the circle icon at the top should be grey not green)
;;
;; Make sure send the Channel 1 (no light in the dashed loudspeaker icon),
;; and turn of the monitoring to hear only the processed sound from the 
;; computer (no light in the headphone icon).
;;
;; The computer mix will go to the USB channel, so put that in your 
;; headphone or the main output.


# for "EnableWasapiOutputs" you can use -1 to have a message prompting
# to use either WASAPI or ASIO for output every time you boot the game
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

# if your game hangs or crashes on exit, try setting "EnableRefCountHack" to true.
# when blank or invalid, the value of "EnableRefCountHack" will be interpreted as
# true if RS ASIO detects the usage of Asio4All.
# the same applies for all inputs.
[Asio.Output]
Driver=GCS-8
BaseChannel=0
AltBaseChannel=
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

[Asio.Input.0]
Driver=GCS-8
Channel=4
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

[Asio.Input.1]
Driver=
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

[Asio.Input.Mic]
Driver=
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=

```