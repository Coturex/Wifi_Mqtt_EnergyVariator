#!/bin/bash
sudo ~/.local/bin/esptool.py --port /dev/ttyUSB0 --baud 230400 write_flash -fm dio -fs 32m 0x00000 ./$1
