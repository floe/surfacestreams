#!/usr/bin/env python3

import sys,gi
gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
gi.require_version('GLib', '2.0')
from gi.repository import Gst, GstBase, GLib

def main(args):

    Gst.init(None)

    pipeline = Gst.Pipeline()

    udpsrc = Gst.ElementFactory.make("audiotestsrc", "audio")
    pipeline.add(udpsrc)

    sink = Gst.ElementFactory.make("autoaudiosink", "sink")
    pipeline.add(sink)

    udpsrc.link(sink)

    pipeline.set_state(Gst.State.PLAYING)

    loop = GLib.MainLoop()
    try:
        loop.run()
    except:
        pass

    pipeline.set_state(Gst.State.NULL)

sys.exit(main(sys.argv))


#queue="queue max-size-time=200000000 leaky=upstream"
#gst-launch-1.0 -vtc udpsrc port=5000 ! application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T ! rtpjitterbuffer do-lost=true ! rtpmp2tdepay ! tsparse set-timestamps=true ! tsdemux name=mux \
#	mux. ! opusparse ! $queue ! opusdec plc=true ! autoaudiosink \
#	mux. ! h264parse ! $queue ! avdec_h264 ! videoconvert ! fpsdisplaysink \
#	mux. ! h264parse ! $queue ! avdec_h264 ! videoconvert ! fpsdisplaysink
