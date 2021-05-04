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
sources = {}
stream = {
    "video_0_0041": "surface",
    "video_0_0042": "front",
    "audio_0_0043": "audio"
}

# conveniently create a new GStreamer element and set parameters
def new_element(element_name,parameters={},myname=None):
    element = Gst.ElementFactory.make(element_name,myname)
    for key,val in parameters.items():
        element.set_property(key,val)
    return element

# add a list of elements to the pipeline and link them in sequence
def add_and_link(elements):
    prev = None
    for item in elements:
        if pipeline.get_by_name(item.name) == None:
            pipeline.add(item)
        item.sync_state_with_parent()
        if prev != None:
            prev.link(item)
        prev = item

# capture and handle bus messages
def bus_call(bus, message, loop):
    t = message.type
    #print(message.src,t)
    if t == Gst.MessageType.EOS:
        print("End-of-stream")
        loop.quit()
    elif t == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        print("Error: %s: %s" % (err, debug))
        loop.quit()
    return True

# a TS demuxer pad has been added
def on_pad_added(src, pad, *user_data):

    name = pad.get_name()
    print("tsdemux pad added: "+name)

    if name.startswith("video"):

        print("adding video subqueue")

        # find corresponding ssrc for the active demuxer
        elname = pad.get_parent_element().get_name()
        teename = sources[elname]+"_"+stream[name]

        add_and_link([ src,
            new_element("h264parse"),
            new_element("queue", { "max-size-time": 200000000, "leaky": "upstream" } ),
            new_element("avdec_h264"),
            new_element("textoverlay",{ "text": teename, "valignment": "top" }),
            new_element("alpha", { "method": "green" } ),
            new_element("tee",{"allow-not-linked":True},myname=teename),
            # uncomment for debug view of individual streams
            #new_element("queue"),
            #new_element("videoconvert"),
            #new_element("fpsdisplaysink",{"sync":False})
        ])

        if stream[name] == "surface":
            # FIXME: hardcoded mixer name
            add_and_link([ pipeline.get_by_name(teename), new_element("queue"), pipeline.get_by_name("videomixer2-0") ])

    if name.startswith("audio"):

        print("adding audio subqueue")

        add_and_link([ src,
            new_element("opusparse"),
            new_element("queue", { "max-size-time": 200000000, "leaky": "upstream" } ),
            new_element("opusdec", { "plc": True } ),
            new_element("autoaudiosink")
        ])

    #pipeline.set_state(Gst.State.PLAYING)
    Gst.debug_bin_to_dot_file(pipeline,Gst.DebugGraphDetails(15),"debug.dot")

# new ssrc coming in on UDP socket, i.e. new client
def on_ssrc_pad(src, pad, *user_data):

    global sources

    name = pad.get_name()
    ssrc = name.split("_")[-1]
    jbname = "rtpjb_"+ssrc
    print("ssrc pad added: "+name)

    # TODO: somehow figure out the peer address from ssrc and/or buffer metadata

    if name.startswith("rtcp"):
        # link the rtcp_src pad to jitterbuffer
        jb = pipeline.get_by_name(jbname)
        sinkpad = jb.request_pad(jb.get_pad_template("sink_rtcp"), None, None)
        pad.link(sinkpad)
        return

    tsdemux = new_element("tsdemux")
    tsdemux.connect("pad-added",on_pad_added)

    add_and_link([ src,
        new_element("rtpjitterbuffer", { "do-lost": True }, jbname ),
        new_element("rtpmp2tdepay"),
        new_element("tsparse", { "set-timestamps": True } ),
        tsdemux
    ])

    # store mapping from demuxer to ssrc for correctly connecting video pads
    sources[tsdemux.get_name()] = "ssrc_"+ssrc

# pad probe for reading metadata
#def probe_callback(pad,info,pdata):
#    print(info.get_buffer())
#    return Gst.PadProbeReturn.OK

def main(args):

    global pipeline

    Gst.init(None)

    pipeline = Gst.Pipeline()

    caps = Gst.Caps.from_string("application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T")

    rtpdemux = new_element("rtpssrcdemux")
    rtpdemux.connect("pad-added",on_ssrc_pad)

    #udpsrc = new_element("udpsrc", { "port": 5000, "retrieve-sender-address": True } )
    #srcpad = udpsrc.get_static_pad("src")
    #srcpad.add_probe(Gst.PadProbeType.BUFFER, probe_callback, None)

    add_and_link([
        new_element("udpsrc", { "port": 5000, "retrieve-sender-address": True } ),
        new_element("capsfilter", { "caps": caps } ),
        rtpdemux
    ])

    add_and_link([ new_element("videomixer"), new_element("videoconvert"), new_element("fpsdisplaysink") ])

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

