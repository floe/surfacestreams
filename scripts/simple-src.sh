#!/bin/bash

# for sanity check, run:
# ffplay -reorder_queue_size 0 -fflags nobuffer -flags low_delay -i rtp://127.0.0.1:5000/

target=${1:-127.0.0.1}
echo "Sending SurfaceStream to target host ${target}, UDP port 5000."
ping -w 1 -q -c 1 $target > /dev/null || echo "Warning: target host ${target} unreachable via ping."

cd "$(dirname $0)"

videoenc="videoconvert ! queue max-size-buffers=1 ! x264enc noise-reduction=10000 speed-preset=ultrafast tune=zerolatency byte-stream=true threads=2 key-int-max=15 ! video/x-h264,profile=baseline ! queue max-size-time=100000000"

command="! videorate ! video/x-raw,width=1280,height=720,framerate=10/1 ! $videoenc ! mux. \
  mpegtsmux name=mux alignment=7 ! rtpmp2tpay ! udpsink ${BIND_ADDRESS} host=${target} port=5000"
	# TODO: lower latency parameter for mpegtsmux?

../realsense "$command"
