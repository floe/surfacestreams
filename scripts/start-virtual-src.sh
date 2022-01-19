#!/bin/bash

cd ../
./surfacecast $1 $2 "videoconvert ! video/x-raw,format=RGB,width=1280,height=720 ! v4l2sink device=/dev/video20"
