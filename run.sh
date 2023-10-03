#!/bin/bash

cd src/threads/build
make clean
make
if [ -n "$1" ]; then
  pintos -- -q run "$1"
else
  pintos -- 
fi
cd ~/os/pintos