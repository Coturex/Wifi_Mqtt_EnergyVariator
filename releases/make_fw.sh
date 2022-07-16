#!/bin/bash
ver=`grep "define FW_VERSION" ../src/main.cpp | awk '{print $3'} | sed s/\"//g`
#hard=`grep "define PCB" ../src/main.cpp | awk '{print $3'} | sed s/\"//g`
echo "firmware :" $ver
cp ../.pio/build/wemos/firmware.bin firmware-$ver

