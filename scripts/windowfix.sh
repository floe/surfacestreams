#!/bin/bash

# wait for window to appear, rename it
until SURF=$(wmctrl -l | egrep 'gst-launch-1.0') ; do sleep 1 ; done
SURF=$(echo $SURF | head -n 1 | cut -f1 -d' ')
wmctrl -i -r $SURF -T surface_sink ; sleep 1

# modify geometry/fullscreen
wmctrl -i -r $SURF -e 0,2100,0,-1,-1 ; sleep 2
wmctrl -i -r $SURF -b add,fullscreen ; sleep 2

# wait for window to appear, rename it
until FACE=$(wmctrl -l | egrep 'gst-launch-1.0') ; do sleep 1 ; done
FACE=$(echo $FACE | head -n 1 | cut -f1 -d' ')
wmctrl -i -r $FACE -T face_sink ; sleep 1

# modify geometry/fullscreen
wmctrl -i -r $FACE -e 0,0,0,-1,-1    ; sleep 2
wmctrl -i -r $FACE -b add,fullscreen ; sleep 2

