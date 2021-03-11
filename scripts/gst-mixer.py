#!/usr/bin/env python3

# with bits and pieces from:
# * https://stackoverflow.com/questions/60230807/convert-gstreamer-pipeline-to-python-code
# * https://gitlab.freedesktop.org/gstreamer/gst-python/-/blob/master/examples/dynamic_src.py

import sys,gi
gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
gi.require_version('GLib', '2.0')
from gi.repository import Gst, GstBase, GLib

pipeline = None

def bus_call(bus, message, loop):
    t = message.type
    print(message.src,t)
    if t == Gst.MessageType.EOS:
        print("End-of-stream")
        loop.quit()
    elif t == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        print("Error: %s: %s" % (err, debug))
        loop.quit()
    return True

def on_pad_added(src, pad, *user_data):
    # Create the rest of your pipeline here and link it 
    name = pad.get_name()
    print("pad_added: "+name)

    if name.startswith("video"):

        print("adding video subqueue")

        parse = Gst.ElementFactory.make("h264parse")
        pipeline.add(parse)
        src.link(parse)
        parse.sync_state_with_parent()

        queue = Gst.ElementFactory.make("queue")
        queue.set_property("max-size-time",200000000)
        queue.set_property("leaky","upstream")
        pipeline.add(queue)
        parse.link(queue)
        queue.sync_state_with_parent()

        avdec = Gst.ElementFactory.make("avdec_h264")
        pipeline.add(avdec)
        queue.link(avdec)
        avdec.sync_state_with_parent()

        convert = Gst.ElementFactory.make("videoconvert")
        pipeline.add(convert)
        avdec.link(convert)
        convert.sync_state_with_parent()

        display = Gst.ElementFactory.make("fpsdisplaysink")
        pipeline.add(display)
        convert.link(display)
        display.sync_state_with_parent()

    if name.startswith("audio"):

        print("adding audio subqueue")

        parse = Gst.ElementFactory.make("opusparse")
        pipeline.add(parse)
        src.link(parse)
        parse.sync_state_with_parent()

        queue = Gst.ElementFactory.make("queue")
        queue.set_property("max-size-time",200000000)
        queue.set_property("leaky","upstream")
        pipeline.add(queue)
        parse.link(queue)
        queue.sync_state_with_parent()

        avdec = Gst.ElementFactory.make("opusdec")
        avdec.set_property("plc",True)
        pipeline.add(avdec)
        queue.link(avdec)
        avdec.sync_state_with_parent()

        output = Gst.ElementFactory.make("autoaudiosink")
        pipeline.add(output)
        avdec.link(output)
        output.sync_state_with_parent()

    pipeline.set_state(Gst.State.PLAYING)
    print(pipeline)


def main(args):

    global pipeline

    Gst.init(None)

    pipeline = Gst.Pipeline()

    udpsrc = Gst.ElementFactory.make("udpsrc")
    udpsrc.set_property("port", 5000)
    pipeline.add(udpsrc)

    caps = Gst.Caps.from_string("application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T")
    filter = Gst.ElementFactory.make("capsfilter")
    filter.set_property("caps", caps)
    pipeline.add(filter)
    udpsrc.link(filter)

    jitbuf = Gst.ElementFactory.make("rtpjitterbuffer")
    jitbuf.set_property("do-lost", True)
    pipeline.add(jitbuf)
    filter.link(jitbuf)

    depay = Gst.ElementFactory.make("rtpmp2tdepay")
    pipeline.add(depay)
    jitbuf.link(depay)

    parse = Gst.ElementFactory.make("tsparse")
    parse.set_property("set-timestamps", True)
    pipeline.add(parse)
    depay.link(parse)

    demux = Gst.ElementFactory.make("tsdemux")
    pipeline.add(demux)
    demux.connect("pad-added",on_pad_added)
    parse.link(demux)

    sink = Gst.ElementFactory.make("fakesink")
    pipeline.add(sink)
    demux.link(sink)

    pipeline.set_state(Gst.State.PLAYING)

    loop = GLib.MainLoop()

    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect ("message", bus_call, loop)

    try:
        loop.run()
    except:
        pass

    pipeline.set_state(Gst.State.NULL)

sys.exit(main(sys.argv))

