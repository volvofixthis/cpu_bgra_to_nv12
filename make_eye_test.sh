#!/bin/sh
./make_test.sh
ffplay -f rawvideo -video_size 1920x1080 -pixel_format nv12 test.nv12
