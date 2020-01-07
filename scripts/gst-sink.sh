gst-launch-1.0 -ve udpsrc port=5000 ! application/x-rtp, media=video, clock-rate=90000, encoding-name=MP2T ! rtpmp2tdepay ! tsparse ! tsdemux name=mux \
	mux. ! queue leaky=downstream ! h264parse ! avdec_h264 ! videoconvert ! fpsdisplaysink sync=false \
	mux. ! queue leaky=downstream ! h264parse ! avdec_h264 ! videoconvert ! fpsdisplaysink sync=false \
	mux. ! queue leaky=downstream ! opusdec ! autoaudiosink sync=false
