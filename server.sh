#!/bin/bash

OUTPUT="fpsdisplaysink sync=false"
#OUTPUT="tee name=split ! fpsdisplaysink sync=false  split. ! queue leaky=downstream ! jpegenc ! rtpgstpay config-interval=1 ! udpsink host=igor06.medien.uni-weimar.de port=6000"
#OUTPUT="jpegenc ! rtpgstpay config-interval=1 ! udpsink host=presence2 port=6000"
#OUTPUT="x264enc ! mp4mux ! filesink location=test.mp4"

# queue leaky=downstream !
INPUT="application/x-rtp ! rtpgstdepay ! jpegdec ! alpha method=green ! mix."

exec gst-launch-1.0 -vt videomixer sync=false background=black name=mix ! videoconvert ! $OUTPUT \
  udpsrc port=5000 ! $INPUT \
  udpsrc port=5001 ! $INPUT \
  udpsrc port=5002 ! $INPUT
