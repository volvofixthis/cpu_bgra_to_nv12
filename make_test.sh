#!/bin/sh
make >/dev/null 2>&1
ffmpeg -loglevel panic -i cool-car-wallpapers-7-2014-hd.jpg -f rawvideo -pix_fmt bgr0 -y test.bgr0
./cpu_convert
./cpu_convert_sse
