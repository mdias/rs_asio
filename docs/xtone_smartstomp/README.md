# XTONE Smart Stomp

Those settings worked on Lenovo Thinkpad X1 Extreme for [XTONE Smart Stomp](https://www.xsonic.cc/XTONE).

I found setting custom buffer size from file more convenient than messing with ASIO4All slider.

In theory, this approach should work for every ASIO4All device. Note that I've disabled all other ASIO4All devices for simplicity

## Rocksmith.ini

```txt
    LatencyBuffer=2
```

## RS_ASIO.ini

```txt
[Asio.Output]
Driver=ASIO4ALL v2
...

[Asio.Input.0]
Driver=ASIO4ALL v2
...
```

![settings](xtone_smartstomp.png "xtone smartstomp")
