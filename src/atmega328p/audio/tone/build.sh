#!/usr/bin/env sh
gcc -DFSAMPL=$1 -DFTONE=$2 -Wall main.c -lm
