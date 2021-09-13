gst-launch-1.0 shmsrc socket-path=/tmp/blah is-live=true do-timestamp=true ! video/x-raw,format=I420,width=1280,height=720,framerate=10/1 ! videoconvert ! fpsdisplaysink
