gst-launch-1.0 -vct v4l2src device=/dev/video20 ! videoconvert ! fpsdisplaysink
#gst-launch-1.0 shmsrc socket-path=/tmp/blah is-live=true do-timestamp=true ! video/x-raw,format=RGB,width=1280,height=720,framerate=15/1 ! videoconvert ! fpsdisplaysink
