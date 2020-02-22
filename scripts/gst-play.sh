#!/bin/bash
gst-launch-1.0 -ve filesrc location=$1 ! tsparse ! tsdemux name=demux \
	demux.video_0_0042 ! h264parse ! queue ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	demux.video_0_0041 ! h264parse ! queue ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	demux.audio_0_0043 ! opusparse ! queue leaky=downstream ! opusdec ! autoaudiosink sync=false
