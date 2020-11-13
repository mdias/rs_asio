# Streaming while using RS ASIO

If you're using RS ASIO for audio output you will find that you cannot stream the audio coming out of the game. This happens because ASIO audio bypasses the windows audio layer and as such other applications cannot detect it.
There are 2 ways to solve this.

### Easy way

The easiest way to solve this is to avoid using the ASIO path for output, **at the cost of potentially a bit higher latency**. You can do this by enabling WASAPI outputs, and disabling the ASIO output in the config file like so (note the empty `Driver=` part in the `Asio.Output` section):

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

### The complicated way

There are ways to do this that will still allow you to maintain the low latency, but this is a lot more complex as you'll have to route ASIO audio signals around until it reaches a virtual input device that is more likely to be useable by your streaming software.

Some guides are available elsewhere to do this kind of thing:

- [lastpixel.tv/low-latency-rocksmith-obs-streaming-with-software-effects/](https://lastpixel.tv/low-latency-rocksmith-obs-streaming-with-software-effects/)
- [raidntrade.com/tutorials/RS-ASIO-VoiceMeeter.html](https://raidntrade.com/tutorials/RS-ASIO-VoiceMeeter.html)

Keep in mind these guides are not maintained by me, and if you need further help setting this up your best bet is to ask for help on [www.reddit/r/rocksmith](https://www.reddit.com/r/rocksmith/)