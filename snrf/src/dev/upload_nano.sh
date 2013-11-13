#!/usr/bin/env sh

/home/texane/repo/arduino/arduino-1.0.5/hardware/tools/avrdude \
-C/home/texane/repo/arduino/arduino-1.0.5/hardware/tools/avrdude.conf \
-v -v -v -v -patmega328p -carduino -P/dev/ttyUSB0 -b57600 -D -Uflash:w:main.hex:i 
