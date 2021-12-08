#!/bin/bash

MODE="v4l2"
CAM="$1"

if [ "$CAM" = "" ] ; then
	MODE="virtcam"
	CAM="null"
fi

#../webcam $MODE $CAM "videoconvert ! video/x-raw,format=RGB,width=1280,height=720 ! v4l2sink device=/dev/video20"
../realsense "videoconvert ! video/x-raw,format=RGB,width=1280,height=720 ! v4l2sink device=/dev/video20"
