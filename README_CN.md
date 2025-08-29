# RS ASIO

此项目旨在为`Rocksmith 2014 Edition - Remastered`（摇滚史密斯2014 - 重制版）提供ASIO驱动支持并避开WASAPI驱动带来的相关问题。

这会在游戏运行时修正代码以干预搜索WASAPI驱动设备的过程，并注入使用ASIO音频API的虚假WASAPI设备。


## 其他语言
[English](README.md)

## 使用方式

- 将[latest release](https://github.com/mdias/rs_asio/releases/latest) (zip archive release-xxx.zip)内的全部内容复制到游戏根目录下
  - 当前仅支持Steam版本的Rocksmith。你可以通过右键Steam游戏库中的Rocksmith，选择“管理”->“浏览本地文件”来打开游戏的根目录
- 修改RS_ASIO.ini来配置使用ASIO音频驱动的设备及其通道等
- 查看[基础配置指南](#基础配置指南)
- 确保Rocksmith.ini设置`ExclusiveMode=1`以及`Win32UltraLowLatencyMode=1`，如果有疑问的话，使用默认配置即可
- 确保你的游戏模式设置为RTC（Real Tone Cable —— 官方专用连接线）而非麦克风模式（[原因](https://github.com/mdias/rs_asio/issues/275#issuecomment-1120386256)）
- 确保你的音频时钟（采样频率）设置为48kHz，RS ASIO会请求使用48kHz模式。你的驱动设备可能并不支持，如果有问题可以尝试手动设置
- 确保你没有使用“NoCableLauncher”（第三方的绕过RTC检查的游戏启动器）或者类似的软件，这可能会导致你的乐器无法被正常检测到
- 另外，游戏根目录下会生成一个日志文件RS_ASIO-log.txt，这可以帮助你找到你的ASIO驱动设备名称或者诊断问题
    - IMPORTANT: Only 32-bit ASIO drivers will be detected!
- 如果你遇到了问题，可以尝试查看[已知问题](#已知问题)来解决

### 如何移除/卸载

- 移除游戏根目录下与本项目相关的所有DLL文件即可

### 在使用RS ASIO的情况下使用流式传输

查看[这篇指南](docs/streaming/README_CN.md)

### 在Linux系统上使用RS ASIO

有些人成功地通过[wineasio](https://www.wineasio.org/)来在Linux上使用RS ASIO。你可以查看[这个issue](https://github.com/mdias/rs_asio/issues/99)来获取更多信息

## 已知可以正常工作的声卡

- [Alesis Core 1](https://github.com/mdias/rs_asio/issues/115)
- [Antelope Audio Zen Go](https://github.com/mdias/rs_asio/issues/294#issuecomment-2212061137)
- [Antelope Audio Zen Tour](https://github.com/mdias/rs_asio/issues/294)
- [Arturia AudioFuse](https://github.com/mdias/rs_asio/issues/114)
- [Arturia MiniFuse 1](https://github.com/mdias/rs_asio/issues/287)
- [Arturia MiniFuse 2](https://github.com/mdias/rs_asio/issues/282)
- [Asus Strix Soar](docs/asus_strix_soar/README.md)
- [Audient Evo 4](docs/audient_evo_4/README.md)
- [Audient Evo 16](docs/audient_evo_4/README.md)
- [Audient iD14 Mk II](https://github.com/mdias/rs_asio/issues/553)
- [Audient iD4](https://github.com/mdias/rs_asio/issues/295)
- Audient iD22
- [Avid Mbox Studio](https://github.com/mdias/rs_asio/issues/467)
- [Behringer GUITAR 2 USB](https://github.com/mdias/rs_asio/issues/246), using ASIO4All
- [Behringer MIC2 USB](docs/behringer_mic2usb/README.md), using ASIO4All
- Behringer U-Phoria UM2  [(see this for more details)](https://github.com/mdias/rs_asio/issues/7) **IMPORTANT: needs legacy ASIO driver**
- [Behringer UMC22](https://github.com/mdias/rs_asio/issues/326)  **IMPORTANT: needs legacy ASIO driver**
- Behringer UMC1820
- Behringer UMC202HD
- [Behringer UMC204](https://github.com/mdias/rs_asio/issues/156)
- [Behringer UMC204HD](https://github.com/mdias/rs_asio/issues/161)
- [Behringer UMC404HD](https://github.com/mdias/rs_asio/issues/13)
- [Behringer XENIX Q502USB](https://github.com/mdias/rs_asio/issues/132) **IMPORTANT: needs legacy ASIO driver**
- [Behringer XR18](https://github.com/mdias/rs_asio/issues/72)
- [BOSS Gigcaster (GCS-8)](docs/boss_gcs_8/README.md)
- [BOSS GT-1](https://github.com/mdias/rs_asio/issues/494)
- [BOSS Katana-Air](https://github.com/mdias/rs_asio/issues/359)
- [BOSS Katana MkII](docs/katana_mk2/README.md)
- [Creative Live! Audio A3](https://github.com/mdias/rs_asio/issues/412)
- [Creative Sound Blaster Z](https://github.com/mdias/rs_asio/issues/191)
- [Darkglass Element](https://github.com/mdias/rs_asio/issues/258)
- [Devine Centro 2i2o](https://github.com/mdias/rs_asio/issues/513#issuecomment-2733642765)
- ESI MAYA22 USB
- [ESI MAYA44 eX](https://github.com/mdias/rs_asio/issues/134) [(Known issues)](#known-issues)
- [ESI U22 XT](https://github.com/mdias/rs_asio/issues/458)
- [ESI UGM96](https://github.com/mdias/rs_asio/issues/250) [(Known issues)](#known-issues)
- [Fender Link I/O](https://github.com/mdias/rs_asio/issues/565)
- [Fender Mustang Micro Plus](https://github.com/mdias/rs_asio/issues/558#issuecomment-3139856047)
- [Focusrite Clarett 2Pre Thunderbolt](https://github.com/mdias/rs_asio/issues/146)
- [Focusrite Clarett 4Pre USB](https://github.com/mdias/rs_asio/issues/42)
- [Focusrite Clarett 8Pre USB](https://github.com/mdias/rs_asio/issues/158)
- [Focusrite Saffire 6 USB 2.0](https://github.com/mdias/rs_asio/issues/116)
- Focusrite Saffire Pro 40
- [Focusrite Scarlett 2i2 1st Gen](https://github.com/mdias/rs_asio/issues/330) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 2nd Gen](https://github.com/mdias/rs_asio/issues/126) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 3rd Gen](https://github.com/mdias/rs_asio/issues/208) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i2 4th Gen](https://github.com/mdias/rs_asio/issues/405#issuecomment-1876314996) [(Known issues)](#known-issues)
- [Focusrite Scarlett 2i4 1st Gen](https://github.com/mdias/rs_asio/issues/133) [(Known issues)](#known-issues)
- Focusrite Scarlett 2i4 2nd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 4i4 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 6i6 2nd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett 8i6 3rd Gen](https://github.com/mdias/rs_asio/issues/208) [(Known issues)](#known-issues)
- [Focusrite Scarlett 16i16 4th Gen](https://github.com/mdias/rs_asio/issues/405#issuecomment-2527560680) [(Known issues)](#known-issues)
- [Focusrite Scarlett 18i8 2nd Gen](docs/focusrite_scarlett_18i8_2nd_gen_OR_asio_link_pro/README.md) [(Known issues)](#known-issues)
- Focusrite Scarlett 18i8 3rd Gen [(Known issues)](#known-issues)
- Focusrite Scarlett 18i20 2nd Gen [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 2nd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [Focusrite Scarlett Solo 3rd Gen](docs/focusrite_solo/README.md) [(Known issues)](#known-issues)
- [Fractal Audio AXE FX III](https://github.com/mdias/rs_asio/issues/347)
- [Hotone Ampero II Stage](https://github.com/mdias/rs_asio/issues/444)
- [Hotone Ampero II Stomp](https://github.com/mdias/rs_asio/issues/444)
- [Hotone Jogg](https://github.com/mdias/rs_asio/issues/533)
- [IK Multimedia AXE I/O](https://github.com/mdias/rs_asio/issues/147)
- [IK Multimedia AXE I/O ONE](https://github.com/mdias/rs_asio/issues/490)
- [IK Multimedia iRig](https://github.com/mdias/rs_asio/issues/164), using ASIO4ALL
- [IK Multimedia iRig HD 2](https://github.com/mdias/rs_asio/issues/117)
- [IK Multimedia iRig HD X](https://github.com/mdias/rs_asio/issues/435)
- [IK Multimedia iRig Pro Duo I/O](docs/irig_pro_duo_io/README.md)
- [JackRouter ASIO driver](https://github.com/mdias/rs_asio/issues/303)
- Juli@ XTe
- Lexicon Alpha
- [Line6 AMPLIFi 75](https://github.com/mdias/rs_asio/issues/97) **Some limitations apply. Follow the link for more information.**
- [Line6 HX Stomp](https://github.com/mdias/rs_asio/issues/251)
- [Line6 POD Go](https://github.com/mdias/rs_asio/pull/171)
- M-Audio 2x2
- [M-Audio AIR 192|4](https://github.com/mdias/rs_asio/issues/98)
- [M-Audio Fast Track 2](https://github.com/mdias/rs_asio/issues/175)
- [M-Audio Fast Track Pro USB](https://github.com/mdias/rs_asio/issues/225) (reporter had clock issues, check link)
- [M-Audio Fast Track Ultra 8R](https://github.com/mdias/rs_asio/issues/135)
- M-Audio M-Track Plus II
- [M-Audio M-Track Solo](https://github.com/mdias/rs_asio/issues/207) NOTE: [there is a report](https://github.com/mdias/rs_asio/issues/562) that the 32-bit drivers are crashing
- [M-Audio M-Track Duo](https://github.com/mdias/rs_asio/issues/207)
- [M-Audio MobilePre mkII](https://github.com/mdias/rs_asio/issues/15)
- [M-Audio ProFire 2626](https://github.com/mdias/rs_asio/issues/212#issuecomment-917706302)
- [Mackie Big Knob Studio+](https://github.com/mdias/rs_asio/issues/385)
- [Mackie Onyx 1620i](https://github.com/mdias/rs_asio/issues/239)
- [Mackie Onyx Artist 1x2](https://github.com/mdias/rs_asio/issues/211)
- [Mackie Onyx Producer 2x2](https://github.com/mdias/rs_asio/issues/211)
- [MIDIPLUS Studio S](docs/midiplus_studio_s/README.md)
- [Miditech AUDIOLINK III](https://github.com/mdias/rs_asio/issues/512), using ASIO4ALL
- [Mooer Steep I](https://github.com/mdias/rs_asio/issues/524), using WineASIO (Steam Deck + Proton 9.0)
- [Monoprice Stage Right STi12](https://github.com/mdias/rs_asio/issues/357)
- MOTU 2408 mk3
- [MOTU M2](https://github.com/mdias/rs_asio/issues/151)
- [MOTU M4](https://github.com/mdias/rs_asio/issues/240)
- MOTU UltraLite AVB
- [MOTU Ultralite mk4](https://github.com/mdias/rs_asio/issues/95)
- [Native Instruments Audio Kontrol 1](https://github.com/mdias/rs_asio/issues/131)
- [Native Instruments Komplete Audio 1](https://github.com/mdias/rs_asio/issues/118)
- [Native Instruments Komplete Audio 2](https://github.com/mdias/rs_asio/issues/120)
- Native Instruments Komplete Audio 6
- [Native Instruments Rig Kontrol 2](https://github.com/mdias/rs_asio/issues/214)
- [Native Instruments Rig Kontrol 3](https://github.com/mdias/rs_asio/issues/536)
- [Neural DSP Nano Cortex](https://github.com/mdias/rs_asio/issues/529)
- [Neural DSP Quad Cortex](https://github.com/mdias/rs_asio/issues/294)
- [NUX Mighty Air](https://github.com/mdias/rs_asio/issues/364)
- [NUX Mighty Lite BT MKII](https://github.com/mdias/rs_asio/issues/446)
- [NUX Mighty Plug](https://github.com/mdias/rs_asio/issues/117)
- [PositiveGrid Spark 40](https://github.com/mdias/rs_asio/issues/218) (issue reported with limited sample rate support in 2024/07; see linked issue)
- [Presonus AudioBox iTwo](https://github.com/mdias/rs_asio/issues/140)
- [Presonus AudioBox USB 96](https://github.com/mdias/rs_asio/issues/140)
- [Presonus ioStation 24c](https://github.com/mdias/rs_asio/issues/542)
- Presonus Quantum 2
- [Presonus Quantum ES 4](https://github.com/mdias/rs_asio/issues/496)
- [Presonus Studio 24c](https://github.com/mdias/rs_asio/issues/280)
- [Presonus StudioLive AR12c](https://github.com/mdias/rs_asio/issues/196)
- ReaRoute ASIO (Reaper virtual ASIO router)
- RME Babyface PRO
- RME HDSPe AIO
- RME Multiface with HDSPe PCIe
- Roland Rubix 22
- Roland Rubix 44
- [Roland UA-1G](https://github.com/mdias/rs_asio/issues/291#issuecomment-1179084242)
- [Roland ua55](docs/roland_ua_55/README.md)
- [Roland V-Studio 100](https://github.com/mdias/rs_asio/issues/91)
- [Røde AI-1](https://github.com/mdias/rs_asio/issues/339)
- [Solid State Logic SSL12](https://github.com/mdias/rs_asio/issues/167)
- [Solid State Logic SSL2+](https://github.com/mdias/rs_asio/issues/167)
- Sonicake Sonic Cube(使用 channel 1 作为输入)
- [SoundCraft Notepad-8FX](https://github.com/mdias/rs_asio/issues/86)
- [SoundCraft Notepad-12FX](https://github.com/mdias/rs_asio/issues/86)
- [SoundCraft Ui24R](https://github.com/mdias/rs_asio/issues/528)
- Sound Devices MixPre-6 II
- [Steinberg CI1](https://github.com/mdias/rs_asio/issues/268)
- [Steinberg IXO12](https://github.com/mdias/rs_asio/issues/495)
- [Steinberg MR816 CSX](https://github.com/mdias/rs_asio/issues/448)
- [Steinberg UR12](docs/steinberg_ur12/README.md)
- [Steinberg UR22](docs/steinberg_ur12/README.md)
- [Steinberg UR22mkII](docs/steinberg_ur12/README.md)
- [Steinberg UR22C](https://github.com/mdias/rs_asio/issues/124)
- [Steinberg UR24C](https://github.com/mdias/rs_asio/issues/341)
- [Steinberg UR44C](https://github.com/mdias/rs_asio/issues/130)
- [Sterling Harmony H224](https://github.com/mdias/rs_asio/issues/390)
- [SuZhou UTECK's Guitar-Cube Chord A](https://github.com/mdias/rs_asio/issues/92)
- Swissonic UA-2x2
- [TASCAM US-1x2](https://github.com/mdias/rs_asio/issues/266)
- [TASCAM US-4x4HR](https://github.com/mdias/rs_asio/issues/402)
- [TC HELICON Blender](https://github.com/mdias/rs_asio/issues/407)
- [Teyun Q22](https://github.com/mdias/rs_asio/issues/520), using ASIO4All
- [Teyun Q24](https://github.com/mdias/rs_asio/issues/427), using ASIO4All
- [Universal Audio Apollo Twin USB](https://github.com/mdias/rs_asio/issues/307) (requires older driver)
- [Universal Audio Volt 1](docs/universal_audio_volt1/README.md)
- [Universal Audio Volt 2 USB](https://github.com/mdias/rs_asio/issues/462)
- [Universal Audio Volt 276](docs/universal_audio_volt_276/README.md)
- [XTONE Smart Stomp](docs/xtone_smartstomp/README.md), using ASIO4All
- [Yamaha AG06 USB Mixing Console](https://github.com/mdias/rs_asio/issues/81)
- [Yamaha THR10II](https://github.com/mdias/rs_asio/issues/210)
- [Yamaha THR30IIW](https://github.com/mdias/rs_asio/issues/210)
- [Zoom AMS-22](https://github.com/mdias/rs_asio/issues/518)
- [Zoom G2.1NU](https://github.com/mdias/rs_asio/issues/400)
- [Zoom G2.1DM](https://github.com/mdias/rs_asio/issues/400)
- Zoom H2N
- [Zoom H6](https://github.com/mdias/rs_asio/issues/198)
- Zoom R24
- [Zoom U-22](https://github.com/mdias/rs_asio/issues/179)
- [Zoom U-44](https://github.com/mdias/rs_asio/issues/334)
- Zoom UAC-2
- [ZOOM UAC-232](https://github.com/mdias/rs_asio/issues/431)

### 基础配置指南

1. 跟着[上面](#使用方式)的步骤来配置
1. 首次运行Rocksmith
1. 打开`RS_ASIO-log.txt`，你可以在这里找到你的ASIO驱动设备列表

```txt
0.456 [INFO]  AsioHelpers::FindDrivers
0.456 [INFO]    ASIO4ALL v2
0.457 [INFO]    MOOER USB Audio
0.457 [INFO]    XMOS USB Audio 2.0 ST 3086
0.457 [INFO]    ZOOM R16_R24 ASIO Driver
```

4. 复制对应的驱动名称到RS_ASIO.ini的[Asio...]部分的Driver选项
1. 再次运行Rocksmith
1. 重复下述步骤直至你听到的音频中没有破音。你应当在没有破音的前提下尽可能设置为更小的值。找到最小的LatencyBuffer然后设置buffer size直至没有破音。
    1. 修改Rocksmith.ini中的LatencyBuffer（尝试4、3、2、1）
    1. 在ASIO驱动控制面板或者RS_ASIO.ini中的CustomBufferSize选项修改buffersize。要注意buffer size应当为32的整数倍。
    1. 运行Rocksmith
    1. 如果遇到问题，检查`RS_ASIO-log.txt`中的日志

### 已知问题

- 你的声卡**必须**支持48kHz的采样率
- 本项目并不提供打开ASIO控制面板的方式，你可能需要自行找到在哪里配置你的声卡
- 在游戏运行过程中修改ASIO设置需要重启游戏来应用（如修改采样频率、采样方式等）
- 某些Focusrite（福克斯特）的声卡设备可能仅会在ASIO的buffer设置为48、96或者192时才能正常输出音频。你可以在RS_ASIO.ini中修改buffer size
    - Changing your windows audio settings to use `2-channel, 24 bit, 48000 Hz (Studio Quality)` format [seems to help with achieving lower buffer sizes](https://github.com/mdias/rs_asio/issues/411).
- [According to reports](https://github.com/mdias/rs_asio/issues?q=label%3A%22focusrite+asio+driver%22+), newer Focusrite driver releases (after 4.102.4) no longer include 32 bit ASIO drivers required by RS ASIO. You can work around this by using software like voicemeeter to reroute audio to the normal 64 bit drivers.
- 某些ESI声卡可能会在退出Rocksmith时卡住，需要将声卡的连接线拔出并重新插入
- 在游戏运行过程中热插拔硬件并不会被游戏识别到
- 在使用ASIO4ALL时游戏有时会崩溃

## 捐赠

你可以在[paypal.me/mdiasdonations](https://paypal.me/mdiasdonations)捐赠这个项目的开发者，不过实际上在issue里说一句“Thanks”就已经足够了。
