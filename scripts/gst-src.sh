#!/bin/bash

# for sanity check, run:
# ffplay -i rtp://127.0.0.1:5000/

cd "$(dirname $0)"

queue="queue max-size-time=200000000 leaky=downstream"
# filesink _might_ need async=false if it blocks?

[ "$1" = "-r" ] && {
	record="fork. ! $queue ! filesink location=$(date '+%Y%m%d-%H%M%S').mpeg"
	shift
}

videoenc1="videoconvert ! $queue ! x264enc noise-reduction=10000 speed-preset=ultrafast tune=zerolatency byte-stream=true threads=2 key-int-max=15 ! video/x-h264,profile=baseline"
videoenc2="videoconvert ! $queue ! x264enc noise-reduction=10000 speed-preset=ultrafast tune=zerolatency byte-stream=true threads=2 key-int-max=15 ! video/x-h264,profile=baseline"

audioenc0="$queue ! opusenc bitrate=8000"

command="                                           ! videorate ! video/x-raw,width=1280,height=720,framerate=10/1 ! $videoenc1 ! mux. \
  v4l2src  do-timestamp=true device=/dev/video-face ! videorate ! video/x-raw,width=640,height=480,framerate=10/1  ! $videoenc2 ! mux. \
  pulsesrc do-timestamp=true                        ! audio/x-raw,channels=1,rate=16000                ! $audioenc0 ! mux. \
  mpegtsmux name=mux ! tee name=fork ! rtpmp2tpay ! udpsink host=${1:-127.0.0.1} port=5000 $record"

gst-launch-1.0 -vtc videotestsrc do-timestamp=true is-live=true $command
#../webcam /dev/video-surf "$command"
