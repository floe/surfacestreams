#!/bin/bash
OUTPUT="fpsdisplaysink sync=false"
#OUTPUT="x264enc ! mp4mux ! filesink location=test.mp4"
exec gst-launch-1.0 -vte videomixer name=mix ! videoconvert ! $OUTPUT \
  udpsrc port=5001 caps=application/x-rtp ! rtpgstdepay ! jpegdec ! mix. \
  udpsrc port=5002 caps=application/x-rtp ! rtpgstdepay ! jpegdec ! alpha method=green ! mix. \
  udpsrc port=5000 caps=application/x-rtp ! rtpgstdepay ! jpegdec ! alpha method=green ! mix.
