#!/bin/bash
gst-launch-1.0 -ve udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=MP2T ! rtpjitterbuffer ! rtpmp2tdepay ! tsparse ! tsdemux name=mux \
	mux. ! h264parse ! queue leaky=downstream ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	mux. ! h264parse ! queue leaky=downstream ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	mux. ! queue leaky=downstream ! opusdec ! autoaudiosink sync=false &
./windowfix.sh
sleep 100d
