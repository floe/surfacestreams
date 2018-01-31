#!/bin/bash

TARGET=${1:-127.0.0.1}
SLOT=${2:-0}

# need udev rules for the logitech cameras, e.g. in /etc/udev/rules.d/99-camera-symlink.rules
FACECAM=/dev/video-face
SURFCAM=/dev/video-surf

# view/stream mjpeg directly from v4l2 device
# note: needs to run BEFORE protonect, otherwise no USB bandwidth left ("no space left on device")
gst-launch-1.0 -e v4l2src device=$FACECAM ! image/jpeg,width=1280,height=720,framerate=30/1 ! rtpgstpay config-interval=1 ! udpsink host=$TARGET port=$((6000+$SLOT)) &

sleep 5

SURFPIPE="rtpgstpay config-interval=1 ! udpsink host=127.0.0.1 port=$((5000+$SLOT))"

if lsusb | grep -qi "045e:02d9" ; then
	# Kinect connected
	../build/bin/Protonect -gstpipe "$SURFPIPE"
elif lsusb | grep -qi "045e:0775" ; then
	# SUR40 connected
	gst-launch-1.0 -e v4l2src device=$SURFCAM ! "video/x-raw,format=GRAY8,width=960,height=540" ! jpegenc ! $SURFPIPE
else
	# macmini with logitech c920
	v4l2-ctl -d $SURFCAM -c focus_absolute=0,focus_auto=0
	./gstreamer $SURFCAM "$SURFPIPE"
fi

