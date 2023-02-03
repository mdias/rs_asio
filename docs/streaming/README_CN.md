# 在使用RS ASIO的情况下使用流式传输

在使用RS ASIO的时候你会发现流式传输无法正常传输游戏的音频输出。这是因为ASIO音频会绕过Windows的音频层来让其他应用程序无法检测到它。

这里有两种解决办法。

### 简易方法

最简单的办法就是不用ASIO来输出音频，但是代价是**会有更大的声音延迟**。你可以通过下述方式来启用WASAPI的音频输出，并在配置文件里禁用ASIO的音频输出(注意在`Asio.Output`部分`Driver=`这里设置为空)：

```ini
[Config]
EnableWasapiOutputs=1
EnableWasapiInputs=0
EnableAsio=1

[Asio]
...

[Asio.Output]
Driver=
...

[Asio.Input.0]
...

[Asio.Input.1]
...
```

### 复杂方法

有几种方法可以让你仍然拥有ASIO的低延迟优势，但是步骤要更复杂些。你需要把ASIO的音频信号转发到一个虚拟的输入设备上，然后就可以被你的串流软件检测到。

这里有一些实现的方法：

- [DeathlySin's tutorial for OBS streaming/recording with ASIO + VoiceMeeter](https://www.reddit.com/r/rocksmith/comments/kv6z9f/rs_asio_guide_including_routing_with_voicemeeter/)
- [lastpixel.tv/low-latency-rocksmith-obs-streaming-with-software-effects/](https://lastpixel.tv/low-latency-rocksmith-obs-streaming-with-software-effects/)
- [raidntrade.com/tutorials/RS-ASIO-VoiceMeeter.html](https://raidntrade.com/tutorials/RS-ASIO-VoiceMeeter.html)

要注意这些指南并不是由我编写的。如果你遇到了问题最好在[www.reddit/r/rocksmith](https://www.reddit.com/r/rocksmith/)上寻求帮助。