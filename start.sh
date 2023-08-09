#!/bin/bash

cd src/threads/build
make clean
make
pintos --
cd ~/os/pintos