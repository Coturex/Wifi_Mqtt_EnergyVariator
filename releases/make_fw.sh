#!/bin/bash
ver=`grep "define FW_VERSION" ../src/main.cpp | awk '{print $3'} | sed s/\"//g`
pcb=`grep "define PCB" ../src/main.cpp | awk '{print $3'} | sed s/\"//g`_pcb
#hard=`grep "define PCB" ../src/main.cpp | awk '{print $3'} | sed s/\"//g`
echo "firmware :" $ver
echo "pcb :" $pcb

cp ../.pio/build/wemos/firmware.bin firmware-$ver-$pcb

