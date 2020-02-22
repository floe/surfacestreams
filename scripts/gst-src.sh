#!/bin/bash
[ "$1" = "-r" ] && {
	record="fork. ! queue ! filesink location=$(date '+%Y%m%d-%H%M%S').mpeg"
	shift
}

videoenc="videoconvert ! queue leaky=downstream ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=2 key-int-max=15"
videoenc2="videoconvert ! queue leaky=downstream ! vaapih264enc bitrate=2000 keyframe-period=10"
command="                              ! video/x-raw,width=1280,height=720,framerate=10/1 ! $videoenc ! mux. \
	v4l2src device=/dev/video-face ! video/x-raw,width=960,height=720,framerate=10/1  ! $videoenc2 ! mux. \
	pulsesrc                       ! audio/x-raw,channels=1,rate=8000                 ! queue leaky=downstream ! opusenc bitrate=8000 ! mux. \
	mpegtsmux name=mux ! tee name=fork ! rtpmp2tpay ! udpsink host=$1 port=5000 $record"

#gst-launch-1.0 -ve videotestsrc is-live=true $command
../webcam /dev/video-surf "$command" &
./windowfix.sh
sleep 100d
