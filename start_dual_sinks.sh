#!/bin/bash

# launch first sink on port 5000
gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! videoflip method=rotate-180 ! videoconvert ! autovideosink &

# wait for window to appear, rename it
until wmctrl -l | grep gst-launch-1.0 ; do sleep 1 ; done
wmctrl -r gst-launch-1.0 -N surface_sink ; sleep 1

# launch second sink on port 6000
#gst-launch-1.0 udpsrc port=6000 caps="application/x-rtp" ! rtpgstdepay ! jpegdec ! videoconvert ! autovideosink &

# wait for window to appear, rename it
#until wmctrl -l | grep gst-launch-1.0 ; do sleep 1 ; done
#wmctrl -r gst-launch-1.0 -N face_sink

# modify geometry/fullscreen for both windows
wmctrl -r surface_sink -e 0,0,1100,-1,-1 ; sleep 2
wmctrl -r surface_sink -b add,fullscreen

#wmctrl -r face_sink -e 0,0,0,-1,-1 
#wmctrl -r face_sink -b add,fullscreen

sleep 100d
