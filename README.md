# How to build
```
$ make clean
$ make CROSS_COMPILE=aarch64-linux-
```

# How to use
```
$ ./tty-wr 
Usage:
    tty-wr <device> <baudrate> <byte1> <byte2> [...]

Exmaple:
    tty-wr /dev/tty_mcu 115200                                         # monitor mode
    tty-wr /dev/tty_mcu 115200 0b 55 aa 00 06 21 06 00 01 ff ff 20
    tty-wr /dev/tty_bp1048 460800 06 55 AA 80 01 A1 20
    tty-wr /dev/tty_58g 57600 55 aa 01 50 00 00                        # 58g read config
    tty-wr /dev/tty_58g 57600 55 aa 01 51 00 00                        # 58g read rf status
```