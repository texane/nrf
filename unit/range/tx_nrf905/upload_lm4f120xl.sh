#!/usr/bin/env sh

HOSTNAME=`hostname`
case $HOSTNAME in
debian)
 LM4F120XL_DIR=/home/texane/repo/stella
 ;;
*)
 LM4F120XL_DIR=/buffer/PCLAB2451/lementec/repo/lm4f120xl
 ;;
esac

$LM4F120XL_DIR/lm4tools/lm4flash/lm4flash -s 0E1068DD main.bin
