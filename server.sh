#!/bin/bash
exec gst-launch-1.0 -vt videomixer name=mix ! videoconvert ! fpsdisplaysink sync=false \
  udpsrc port=5001 caps=application/x-rtp ! rtpgstdepay ! jpegdec ! mix. \
	udpsrc port=5002 caps=application/x-rtp ! rtpgstdepay ! jpegdec ! alpha method=green ! mix. \
  udpsrc port=5000 caps=application/x-rtp ! rtpgstdepay ! jpegdec ! alpha method=green ! mix.
