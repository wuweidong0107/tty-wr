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
    tty-wr /dev/tty_mcu 115200 0b 55 aa 00 06 21 06 00 01 ff ff 20
```