#!/bin/bash
cd "$(dirname $0)"
gst-launch-1.0 -vtc udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=MP2T ! rtpjitterbuffer ! rtpmp2tdepay ! tsparse ! tsdemux name=mux \
	mux. ! h264parse ! queue leaky=downstream ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	mux. ! h264parse ! queue leaky=downstream ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	mux. ! opusparse ! queue leaky=downstream ! opusdec ! autoaudiosink &
#./windowfix.sh
sleep 100d
