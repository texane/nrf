#!/usr/bin/env sh

HOSTNAME=`hostname`
case $HOSTNAME in
debian)
 LM4F120XL_DIR=$HOME/repo/stella
 ;;
*)
 LM4F120XL_DIR=/buffer/PCLAB2451/lementec/repo/lm4f120xl
 ;;
esac

$LM4F120XL_DIR/toolchain/bin/arm-none-eabi-gcc \
-g -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp \
-Os -ffunction-sections -fdata-sections -Wno-unused-function \
-MD -std=c99 -Wall -pedantic -DPART_LM4F120H5QR -c -I$LM4F120XL_DIR/lib \
-DTARGET_IS_BLIZZARD_RA1 \
-DCONFIG_LM4F120XL=1 \
main.c startup_gcc.c $LM4F120XL_DIR/lib/utils/uartstdio.c

$LM4F120XL_DIR/toolchain/bin/arm-none-eabi-ld \
-T lm4f120xl.ld --entry ResetISR \
-o a.out \
startup_gcc.o main.o uartstdio.o \
$LM4F120XL_DIR/lib/driverlib/gcc-cm4f/libdriver-cm4f.a \
--gc-sections

$LM4F120XL_DIR/toolchain/bin/arm-none-eabi-objcopy \
-O binary a.out main.bin
