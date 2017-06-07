# various gstreamer commands for streaming/mixing the raw data from Kinect v2

# source with (motion?) jpeg
gst-launch-1.0 videotestsrc pattern=smpte ! "video/x-raw,format=BGRx,width=1280,height=720" ! jpegenc ! rtpgstpay config-interval=1 ! udpsink host=127.0.0.1 port=5000

# plain sink
gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! videoconvert ! autovideosink

# sink with chroma-key on green
gst-launch-1.0 -vt videomixer name=mix ! videoconvert ! fpsdisplaysink sync=false \
  videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! mix. \
	udpsrc port=5000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! alpha method=green ! mix.

# view/stream mjpeg directly from v4l2 device
gst-launch-1.0 -e v4l2src device=/dev/video0 ! image/jpeg,width=1280,height=720,framerate=30/1 ! jpegdec ! autovideosink
gst-launch-1.0 -e v4l2src device=/dev/video0 ! image/jpeg,width=1280,height=720,framerate=30/1 ! rtpgstpay config-interval=1 ! udpsink host=127.0.0.1 port=5001
