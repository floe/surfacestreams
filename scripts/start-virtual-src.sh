#!/bin/bash

v4l2-ctl -d /dev/video-surf -c focus_auto=0
v4l2-ctl -d /dev/video-surf -c focus_absolute=0

cd ../
rm /tmp/blah*
./surfacecast $1 $2 "videoconvert ! video/x-raw,format=RGB,width=1280,height=720 ! v4l2sink device=/dev/video20"
#./surfacecast  $1 $2 "videoconvert ! video/x-raw,format=RGB,width=1280,height=720 ! shmsink socket-path=/tmp/blah wait-for-connection=false"
rm /tmp/blah*
