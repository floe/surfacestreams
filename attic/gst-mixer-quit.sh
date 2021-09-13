#!/bin/bash
gst-launch-1.0 videotestsrc num-buffers=10 ! x264enc ! mpegtsmux ! rtpmp2tpay ssrc=0 ! udpsink host=$1 port=5000
