#!/usr/bin/env sh

$HOME/repo/stella/toolchain/bin/arm-none-eabi-gcc \
-g -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -Os -ffunction-sections -fdata-sections -Wno-unused-function \
-MD -std=c99 -Wall -pedantic -DPART_LM4F120H5QR -c -I$HOME/repo/stella/lib \
-DTARGET_IS_BLIZZARD_RA1 \
-DCONFIG_LM4F120XL=1 \
main.c startup_gcc.c $HOME/repo/stella/lib/utils/uartstdio.c

$HOME/repo/stella/toolchain/bin/arm-none-eabi-ld \
-T main.ld --entry ResetISR \
-o a.out \
startup_gcc.o main.o uartstdio.o \
$HOME/repo/stella/lib/driverlib/gcc-cm4f/libdriver-cm4f.a \
--gc-sections

$HOME/repo/stella/toolchain/bin/arm-none-eabi-objcopy \
-O binary a.out main.bin
