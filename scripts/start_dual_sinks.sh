#!/bin/bash

# launch first sink on port 5000
#gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! videoflip method=rotate-180 ! videoconvert ! autovideosink &
gst-launch-1.0 videotestsrc ! autovideosink &

# wait for window to appear, rename it
until SURF=$(wmctrl -l | grep gst-launch-1.0) ; do sleep 1 ; done
SURF=$(echo $SURF | cut -f1 -d' ')
wmctrl -i -r $SURF -T surface_sink ; sleep 1

# modify geometry/fullscreen
wmctrl -i -r $SURF -e 0,0,1100,-1,-1 ; sleep 2
wmctrl -i -r $SURF -b add,fullscreen ; sleep 2

# launch second sink on port 6000
#gst-launch-1.0 udpsrc port=6000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! videoconvert ! autovideosink &
gst-launch-1.0 videotestsrc ! autovideosink &

# wait for window to appear, rename it
until FACE=$(wmctrl -l | grep gst-launch-1.0) ; do sleep 1 ; done
FACE=$(echo $FACE | cut -f1 -d' ')
wmctrl -i -r $FACE -T face_sink ; sleep 1

# modify geometry/fullscreen
wmctrl -i -r $FACE -e 0,0,0,-1,-1    ; sleep 2
wmctrl -i -r $FACE -b add,fullscreen ; sleep 2

sleep 100d
