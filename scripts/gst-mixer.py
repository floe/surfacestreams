#!/usr/bin/env python3

# with bits and pieces from:
# * https://stackoverflow.com/questions/60230807/convert-gstreamer-pipeline-to-python-code
# * https://gitlab.freedesktop.org/gstreamer/gst-python/-/blob/master/examples/dynamic_src.py

import sys,gi
gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
gi.require_version('GstNet', '1.0')
gi.require_version('GLib', '2.0')
from gi.repository import Gst, GstBase, GstNet, GLib

# collection of all data for one client
class Client:
    def __init__(self):
        self.ip = ""
        self.tee = None
        self.mixer = None
    def __repr__(self):
        return("Client "+self.ip+": Tee "+str(self.tee)+", Mixer: "+str(self.mixer))

pipeline = None
clients = { }
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
        ssrc = src.get_name().split("_")[-1]
        teename = "tee_"+ssrc+"_"+stream[name]

        mytee = new_element("tee",{"allow-not-linked":True},myname=teename)

        add_and_link([ src,
            new_element("h264parse"),
            new_element("queue", { "max-size-time": 200000000, "leaky": "upstream" } ),
            new_element("avdec_h264"),
            new_element("textoverlay",{ "text": teename, "valignment": "top" }),
            new_element("alpha", { "method": "green" } ),
            mytee,
            # uncomment for debug view of individual streams
            #new_element("queue"),
            #new_element("videoconvert"),
            #new_element("fpsdisplaysink",{"sync":False})
        ])

        if stream[name] == "surface":

            clients[ssrc].tee = mytee

            # create mixers where needed (but only if there are at least 2 clients)
            if len(clients) > 1:
                for c in clients:
                    client = clients[c]
                    if client.mixer != None:
                        continue
                    # TODO: add encoders instead of displaysink
                    tmpmixer = new_element("videomixer",myname="mixer_"+c)
                    add_and_link([ tmpmixer, new_element("videoconvert"), new_element("fpsdisplaysink") ])
                    client.mixer = tmpmixer

            # get mixer for own ssrc
            mymixer = pipeline.get_by_name("mixer_"+ssrc)

            for c in clients:

                client = clients[c]
                print(c,client)

                if c == ssrc: # skip own ssrc
                    continue
                
                # for every _other_ mixer, link my tee to that mixer
                add_and_link([ mytee, new_element("queue"), client.mixer ])

                # for every _other_ tee, link that tee to my mixer
                add_and_link([ client.tee, new_element("queue"), mymixer ])

        elif stream[name] == "front":
            # TODO: implement the same for front cam streams
            pass

    if name.startswith("audio"):

        print("adding audio subqueue")
        # TODO: implement audio mixing

        add_and_link([ src,
            new_element("opusparse"),
            new_element("queue", { "max-size-time": 200000000, "leaky": "upstream" } ),
            new_element("opusdec", { "plc": True } ),
            new_element("autoaudiosink")
        ])

    # write out debug dot file (needs envvar GST_DEBUG_DUMP_DOT_DIR set)
    Gst.debug_bin_to_dot_file(pipeline,Gst.DebugGraphDetails(15),"debug.dot")

# new ssrc coming in on UDP socket, i.e. new client
def on_ssrc_pad(src, pad, *user_data):

    global clients

    name = pad.get_name()
    ssrc = name.split("_")[-1]
    jbname = "rtpjb_"+ssrc
    print("ssrc pad added: "+name)

    if name.startswith("rtcp"):
        # link the rtcp_src pad to jitterbuffer
        jb = pipeline.get_by_name(jbname)
        sinkpad = jb.request_pad(jb.get_pad_template("sink_rtcp"), None, None)
        pad.link(sinkpad)
        return

    # add pad probe for buffer metadata (which contains sender IP address)
    pad.add_probe(Gst.PadProbeType.BUFFER, probe_callback, None)

    tsdemux = new_element("tsdemux",myname="tsd_"+ssrc)
    tsdemux.connect("pad-added",on_pad_added)

    add_and_link([ src,
        new_element("rtpjitterbuffer", { "do-lost": True }, jbname ),
        new_element("rtpmp2tdepay"),
        new_element("tsparse", { "set-timestamps": True } ),
        tsdemux
    ])

    # store client metadata
    client = Client()
    #client.mixer = mymixer
    clients[ssrc] = client

# pad probe for reading address metadata
def probe_callback(pad,info,pdata):
    global clients
    buf = info.get_buffer()
    foo = GstNet.buffer_get_net_address_meta(buf)
    addr = foo.addr.get_address().to_string() #+":"+str(foo.addr.get_port())
    ssrc = pad.get_name().split("_")[-1]
    if ssrc in clients:
        clients[ssrc].ip = addr
    return Gst.PadProbeReturn.OK

def main(args):

    global pipeline

    Gst.init(None)

    pipeline = Gst.Pipeline()

    caps = Gst.Caps.from_string("application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T")

    # setup demuxer with callback for new ssrc
    rtpdemux = new_element("rtpssrcdemux")
    rtpdemux.connect("pad-added",on_ssrc_pad)

    # setup udp src with pad probe to retrieve address metadata
    udpsrc = new_element("udpsrc", { "port": 5000, "retrieve-sender-address": True } )

    # initial pipeline: udpsrc -> caps -> rtpdemux
    add_and_link([ udpsrc, new_element("capsfilter", { "caps": caps } ), rtpdemux ])

    # kick things off
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

