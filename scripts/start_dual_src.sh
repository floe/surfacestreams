#!/bin/bash

TARGET=${1:-127.0.0.1}
SLOT=${2:-0}

# need udev rules for the logitech cameras, e.g. in /etc/udev/rules.d/99-camera-symlink.rules
FACECAM=/dev/video-face
SURFCAM=/dev/video-surf

# launch audio stream
gst-launch-1.0 -e pulsesrc ! rtpgstpay config-interval=1 ! udpsink host=$TARGET port=$((7000+$SLOT)) &

# view/stream mjpeg directly from v4l2 device
# note: needs to run BEFORE protonect, otherwise no USB bandwidth left ("no space left on device") - limiting to 15fps to save more bandwidth
gst-launch-1.0 -e v4l2src device=$FACECAM ! image/jpeg,width=1280,height=720,framerate=15/1 ! rtpgstpay config-interval=1 ! udpsink host=$TARGET port=$((6000+$SLOT)) &

sleep 5

SURFPIPE="jpegenc quality=75 ! rtpgstpay config-interval=1 ! udpsink host=$TARGET port=$((5000+$SLOT))"

if lsusb | grep -qi "045e:02d9" ; then
	# Kinect connected
	../build/bin/Protonect -gstpipe "videorate ! video/x-raw,framerate=15/1 ! $SURFPIPE"
elif lsusb | grep -qi "045e:0775" ; then
	# SUR40 connected
	gst-launch-1.0 -e v4l2src device=/dev/v4l-touch0 ! "video/x-raw,format=GRAY8,width=960,height=540" ! $SURFPIPE
else
	# macmini with logitech c920
	v4l2-ctl -d $SURFCAM -c focus_auto=0
	v4l2-ctl -d $SURFCAM -c focus_absolute=0
	./gstreamer $SURFCAM "$SURFPIPE"
fi

