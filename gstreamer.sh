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
# note: needs to run BEFORE protonect, otherwise no USB bandwidth left ("no space left on device")
gst-launch-1.0 -e v4l2src device=/dev/video0 ! image/jpeg,width=1280,height=720,framerate=30/1 ! jpegdec ! autovideosink
gst-launch-1.0 -e v4l2src device=/dev/video0 ! image/jpeg,width=1280,height=720,framerate=30/1 ! rtpgstpay config-interval=1 ! udpsink host=127.0.0.1 port=5001

# 3 mixers with 3 sources :-O
gst-launch-1.0 -vt \
  videomixer name=mix1 ! videoconvert ! fpsdisplaysink sync=false \
  videomixer name=mix2 sink_0::zorder=1 sink_1::zorder=0 ! videoconvert ! fpsdisplaysink sync=false \
  videomixer name=mix3 ! videoconvert ! fpsdisplaysink sync=false \
  videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! tee name=snow ! queue ! mix1. \
  udpsrc port=5000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! alpha method=green ! tee name=net ! queue ! mix1. \
  videotestsrc pattern=colors ! video/x-raw,width=1280,height=720 ! alpha method=green ! tee name=col ! queue ! mix2. \
  col.  ! queue ! mix3. \
  snow. ! queue ! mix2. \
  net.  ! queue ! mix3.

