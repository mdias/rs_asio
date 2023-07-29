# Ubuntu 12.04 - Steam Proton setup


### Configuring Steam

1. Install the `steam-installer` package. DO NOT install the Steam snap version.
2. Open steam and login with your steam account
	- It's recommended you launch steam through a command terminal to see extra log information.
3. Go to menu `Steam -> Settings`, and on the `Compatibility` tab switch on the option `Enable Steam Play for all other titles`
4. Install Rocksmith 2014
	- Since we didn't configure anything else, we should be using the `Proton Experimental` version at this point.
5. Run the game once to check that the game boots up fine

### Installing WineASIO

In order to install WineASIO we must first add the repositories that contain the `wineasio` package:

1. Go to [https://kx.studio/Repositories](https://kx.studio/Repositories) and download the `kxstudio-repos.deb` file
2. Install the deb file you just downloaded
3. Open a command terminal and execute `sudo apt-get update`

Now you should be able to install WineASIO.

NOTE: we're assuming your steam is installed in `~/.steam/debian-installation/`. If that's not the case for you, change the relevant commands accordingly.

1. Open a command terminal and execute `sudo apt-get install wineasio`
2. Now we need to make the wineasio files available for the installation of the game
	- Copy `/usr/lib/i386-linux-gnu/wine/wineasio.dll.so` to `~/.steam/debian-installation/steamapps/common/Proton - Experimental/files/lib/wine/i386-unix`
	- Copy `/usr/lib/i386-linux-gnu/wine/wineasio.dll` to `~/.steam/debian-installation/steamapps/common/Proton - Experimental/files/lib/wine/i386-windows`
3. Make sure the owner of the files is your user, for example:
	- `chown your_user:your_user ~/.steam/debian-installation/steamapps/common/Proton\ -\ Experimental/files/lib/wine/i386-unix/wineasio.dll.so`
	- `chown your_user:your_user ~/.steam/debian-installation/steamapps/common/Proton\ -\ Experimental/files/lib/wine/i386-windows/wineasio.dll`
4. Make sure permissions are `r-xr-xr-x` for the files, for example:
	- `chmod 555 ~/.steam/debian-installation/steamapps/common/Proton\ -\ Experimental/files/lib/wine/i386-unix/wineasio.dll.so`
	- `chmod 555 ~/.steam/debian-installation/steamapps/common/Proton\ -\ Experimental/files/lib/wine/i386-windows/wineasio.dll`

### Installing RS-ASIO

- Copy the contents (`avrt.dll`, `RS_ASIO.dll`, `RS_ASIO.ini`) of [latest release](https://github.com/mdias/rs_asio/releases/latest) (zip archive release-xxx.zip) to the game folder.
	- The rest of the guide assumes game folder is `~/.steam/debian-installation/steamapps/common/Rocksmith2014`
- Run the game
	- The game should now complain that it has no available audio devices
	- Close the game
	- The `RS_ASIO-log.txt` file has been generated in the game folder, if you open it you should see the following in the log:
```
...
0.397 [INFO]  GetWineAsioInfo - Looking for wineasio.dll... 
0.422 [INFO]    loaded
0.422 [INFO]    path: C:\windows\system32\wineasio.dll
0.422 [INFO]    name: wineasio-rsasio
...
```

If you see these entries in the log, that means RS-ASIO is able to locate and load your WineASIO installation. However this doesn't mean that WineASIO itself will be able to initialize properly.

NOTE: RS-ASIO has functionality to try to find wineasio without having to use `regsvr32` to register the wineasio dll. This is the message you see in the log entries above. When it finds it, it will expose it with driver name `wineasio-rsasio`, but if you also registered the wineasio.dll manually, you might have both `wineasio` and `wineasio-rsasio` entries showing on your log; this is not a problem and you can use either, but the `wineasio-rsasio` one has the guarantee that the file exists as RS-ASIO just found it.

### Configuring RS-ASIO.ini

A typical `RS-ASIO.ini` for `wineasio` will look like this:
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
;BufferSizeMode=custom
;CustomBufferSize=
BufferSizeMode=driver

[Asio.Output]
Driver=wineasio-rsasio
BaseChannel=0
AltBaseChannel=
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=false

[Asio.Input.0]
Driver=wineasio-rsasio
Channel=0
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=false

[Asio.Input.1]
Driver=wineasio-rsasio
Channel=1
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=false

[Asio.Input.Mic]
Channel=
EnableSoftwareEndpointVolumeControl=1
EnableSoftwareMasterVolumeControl=1
SoftwareMasterVolumePercent=100
EnableRefCountHack=false
```

If you run the game now RS-ASIO will attempt to use WineASIO, which may or may not succeed depending on whether you have it (and jackd) configured correctly. If you launched steam  from a command line terminal you should also see some entries there about wineasio attempting to connect to jackd etc.