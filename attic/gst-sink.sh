#!/bin/bash
cd "$(dirname $0)"

queue="queue max-size-time=200000000 leaky=upstream"
# maybe add rtpssrcdemux between jitterbuf and mp2ts-depay

gst-launch-1.0 -vtc udpsrc port=5000 ! application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T ! rtpjitterbuffer do-lost=true ! rtpmp2tdepay ! tsdemux name=mux \
	mux. ! opusparse ! $queue ! opusdec plc=true ! autoaudiosink \
	mux. ! h264parse ! $queue ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	mux. ! h264parse ! $queue ! avdec_h264 ! videoconvert ! fpsdisplaysink
#./windowfix.sh
#sleep 100d
