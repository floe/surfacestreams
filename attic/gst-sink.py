#!/usr/bin/env python3

# with bits and pieces from:
# * https://stackoverflow.com/questions/60230807/convert-gstreamer-pipeline-to-python-code
# * https://gitlab.freedesktop.org/gstreamer/gst-python/-/blob/master/examples/dynamic_src.py

import sys,gi
gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
gi.require_version('GstVideo', '1.0')
gi.require_version('GLib', '2.0')
from gi.repository import Gst, GstBase, GstVideo, GLib


# global objects
pipeline = None

# magic SSRC values with special meaning (main stream, exit)
main_ssrc = "123"
exit_ssrc = "0"

# default parameters for x264enc components
x264params = {"noise-reduction":10000, "speed-preset":"ultrafast", "tune":"zerolatency", "byte-stream":True,"threads":2, "key-int-max":15}


# conveniently create a new GStreamer element and set parameters
def new_element(element_name,parameters={},myname=None):
    element = Gst.ElementFactory.make(element_name,myname)
    for key,val in parameters.items():
        element.set_property(key,val)
    return element

# convenience function to add a list of elements to the pipeline and link them in sequence
def add_and_link(elements):
    prev = None
    for item in elements:
        if item == None:
            continue
        if pipeline.get_by_name(item.name) == None:
            pipeline.add(item)
        item.sync_state_with_parent()
        if prev != None:
            prev.link(item)
        prev = item

# capture and handle bus messages
def bus_call(bus, message, loop):
    t = message.type
    #print(message,message.src,t)
    if t == Gst.MessageType.EOS:
        print("End-of-stream, quitting.\n")
        loop.quit()
    elif t == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        print("Error: %s: %s\n" % (err, debug))
        loop.quit()
    elif t == Gst.MessageType.WARNING:
        err, debug = message.parse_warning()
        print("Warning: %s: %s\n" % (err, debug))
    elif t == Gst.MessageType.NEW_CLOCK:
        print("New clock source selected.\n")
    elif t == Gst.MessageType.CLOCK_LOST:
        print("Clock lost!\n")
    return True

def event_probe(pad,info,pdata):
    event = info.get_event()
    evtype = GstVideo.navigation_event_get_type(event)
    if evtype == GstVideo.NavigationEventType.MOUSE_MOVE:
        foo = GstVideo.navigation_event_parse_mouse_move_event(event)
        print(foo)
    if evtype == GstVideo.NavigationEventType.KEY_PRESS or evtype == GstVideo.NavigationEventType.KEY_RELEASE:
        foo = GstVideo.navigation_event_parse_key_event(event)
        print(foo)
    if evtype == GstVideo.NavigationEventType.MOUSE_BUTTON_PRESS or evtype == GstVideo.NavigationEventType.MOUSE_BUTTON_RELEASE:
        foo = GstVideo.navigation_event_parse_mouse_button_event(event)
        print(foo)
    return Gst.PadProbeReturn.OK

def main(args):

    global pipeline

    print("SurfaceCast client output component 0.1")

    Gst.init(None)

    pipeline = Gst.parse_launch("udpsrc port=5000 ! application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T ! rtpjitterbuffer do-lost=true ! rtpmp2tdepay ! tsdemux name=mux \
	mux. ! opusparse ! queue ! opusdec plc=true ! autoaudiosink \
	mux. ! h264parse ! queue ! avdec_h264 ! videoconvert ! fpsdisplaysink \
	mux. ! h264parse ! queue ! avdec_h264 ! videoconvert ! fpsdisplaysink")

    # kick things off
    pipeline.set_state(Gst.State.PLAYING)
    loop = GLib.MainLoop()

    # listen for bus messages
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect ("message", bus_call, loop)

    # listen for pad events sent by the displaysinks
    src = pipeline.get_by_name("mux")
    pad = src.get_static_pad("sink")
    pad.add_probe(Gst.PadProbeType.EVENT_UPSTREAM,event_probe,None)

    try:
        loop.run()
    except:
        pass

    pipeline.set_state(Gst.State.NULL)

sys.exit(main(sys.argv))

