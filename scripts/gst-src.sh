gst-launch-1.0 -ve mpegtsmux name=mux ! rtpmp2tpay ! udpsink host=$1 port=5000 \
	v4l2src                   ! video/x-raw,width=1280,height=720                ! videoconvert ! queue leaky=downstream ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=2 key-int-max=15 ! mux. \
	videotestsrc is-live=true ! video/x-raw,width=1280,height=720,framerate=10/1 ! videoconvert ! queue leaky=downstream ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=2 key-int-max=15 ! mux. \
	pulsesrc                  ! audio/x-raw,channels=1,rate=8000                 ! queue leaky=downstream ! opusenc bitrate=8000 ! mux.
