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

    def __init__(self,ssrc):
        self.ip = ""
        self.audio_mixer = None
        self.audio_tee = None
        self.surface_tee = None
        self.front_tee = None
        self.front_linked = False
        self.surface_mixer = None
        self.muxer = None
        self.ssrc = ssrc

    def __repr__(self):
        return("Client "+self.ip+": Tee "+str(self.tee)+", Mixer: "+str(self.surface_mixer))

    # try to avoid race condition where idle loop runs before all streams for client have been added
    def ready(self):
        return self.audio_tee != None and self.surface_tee != None and self.front_tee != None

    # link client to frontmixer
    def link_to_front(self):

        if self.front_linked or self.front_tee == None:
            return

        print("    linking client "+self.ssrc+" to frontmixer")
        self.front_linked = True

        # request and link pads from tee and frontmixer
        sinkpad = link_request_pads(self.front_tee,"src_%u",frontmixer,"sink_%u")
        #sinkpad.set_property("max-last-buffer-repeat",10000000000) # apparently not needed

        # set xpos/ypos properties on pad according to sequence number
        # FIXME: only works with <= 4 clients at the moment
        padnum = int(sinkpad.get_name().split("_")[1])
        sinkpad.set_property("xpos",offsets[padnum][0])
        sinkpad.set_property("ypos",offsets[padnum][1])

    # create surface mixer for client
    def create_surface_mixer(self):

        if self.surface_mixer != None:
            return

        print("    creating surface mixer for client "+self.ssrc+", streaming to "+self.ip+":5000")

        self.surface_mixer = new_element("compositor",{"background":"black"},myname="mixer_"+self.ssrc)
        self.muxer = new_element("mpegtsmux", {"alignment":7}, myname="muxer_"+self.ssrc)

        add_and_link([
            self.surface_mixer,
            new_element("videoconvert"),
            new_element("queue",{"max-size-buffers":1}),
            new_element("x264enc",x264params),
            new_element("capsfilter",{"caps":Gst.Caps.from_string("video/x-h264,profile=baseline")}),
            self.muxer,
            new_element("rtpmp2tpay"),
            new_element("udpsink",{"host":self.ip,"port":5000})
        ])

        # link frontstream tee to client-specific muxer
        link_request_pads(frontstream,"src_%u",self.muxer,"sink_%d")

    # TODO: merge into single create_mixers() function?
    # create audio mixer for client
    def create_audio_mixer(self):

        if self.muxer == None:
            return
        if self.audio_mixer != None:
            return

        print("    creating audio mixer for client "+self.ssrc)

        self.audio_mixer = new_element("audiomixer",myname="audio_"+self.ssrc)

        add_and_link([
            self.audio_mixer,
            new_element("queue",{"max-size-time":100000000}), # TODO: queue parameters?
            new_element("opusenc",{"bitrate":16000}),
            self.muxer
        ])

    # helper function to link source tees to destination mixers
    # TODO: maybe use a dedicated dict instead of getattr?
    def link_streams_oneway(self,dest,prefix,qparams):
        if not prefix+self.ssrc+"_"+dest.ssrc in mixer_links:
            print("    linking client "+self.ssrc+" to "+prefix+"mixer "+dest.ssrc)
            add_and_link([
                getattr(self,prefix+"tee"),
                new_element("queue",qparams),
                getattr(dest,prefix+"mixer")
            ])
            mixer_links.append(prefix+self.ssrc+"_"+dest.ssrc)

    # link all other clients to this mixer, this client to other mixers
    def link_streams(self,prefix,qparams):

        for c in clients:

            if c == self.ssrc: # skip own ssrc
                continue

            other = clients[c]

            # for every _other_ mixer, link my tee to that mixer
            self.link_streams_oneway(other,prefix,qparams)

            # for every _other_ tee, link that tee to my mixer
            other.link_streams_oneway(self,prefix,qparams)

    # link all other clients to this mixer, this client to other mixers
    def link_surface_streams(self):
        self.link_streams("surface_",{"max-size-buffers":1})
        return

    # link all other audio streams to this mixer, this client to other mixers
    def link_audio_streams(self):
        self.link_streams("audio_",{"max-size-time":100000000})
        return

    # create video decoding subqueue
    def create_video_decoder(self,src,teename):

        print("  creating video decoding subqueue")

        mytee = new_element("tee",{"allow-not-linked":True},myname=teename)

        alpha = None
        if teename.endswith("surface"):
            alpha = new_element("alpha", { "method": "green" } )

        add_and_link([
            src,
            new_element("h264parse"),
            new_element("queue",{"max-size-time":100000000}),
            new_element("avdec_h264"),
            # NOTE: make textoverlay configurable for debug output
            #new_element("textoverlay",{ "text": teename, "valignment": "top" }),
            alpha,
            mytee,
            # NOTE: make debug view of individual streams configurable
            #new_element("queue"),
            #new_element("videoconvert"),
            #new_element("fpsdisplaysink",{"sync":False})
        ])

        # store reference to tee in client
        if teename.endswith("surface"):
            self.surface_tee = mytee
        elif teename.endswith("front"):
            self.front_tee = mytee

    # create audio decoding subqueue
    def create_audio_decoder(self,src,teename):

        print("  creating audio decoding subqueue")

        self.audio_tee = new_element("tee",{"allow-not-linked":True},myname=teename)

        add_and_link([
            src,
            new_element("opusparse"),
            new_element("queue",{"max-size-time":100000000}),
            new_element("opusdec", { "plc": True } ),
            self.audio_tee
        ])



pipeline = None
frontmixer = None
frontstream = None

# list of new clients that need to be linked
new_client = []

# links between individual client tee/mixer pairs
mixer_links = []

# list of clients
clients = { }

# mapping from transport stream to readable name
stream = {
    "video_0_0041": "surface",
    "video_0_0042": "front",
    "audio_0_0043": "audio"
}

# position offsets for 4 front streams
# FIXME: how to handle > 4 clients?
offsets = [
    (640,360),
    (  0,  0),
    (640,  0),
    (  0,360)
]

# default parameters for x264enc components
x264params = {"noise-reduction":10000, "speed-preset":"ultrafast", "tune":"zerolatency", "byte-stream":True,"threads":2, "key-int-max":15}

# default parameters for queues
queueparams = { "max-size-time": 1000000000 }


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
    #print(message.src,t)
    if t == Gst.MessageType.EOS:
        print("End-of-stream")
        loop.quit()
    elif t == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        print("Error: %s: %s" % (err, debug))
        loop.quit()
    elif t == Gst.MessageType.WARNING:
        err, debug = message.parse_warning()
        print("Warning: %s: %s" % (err, debug))
    elif t == Gst.MessageType.NEW_CLOCK:
        print("New clock source selected.")
    elif t == Gst.MessageType.CLOCK_LOST:
        print("Clock lost!")
    return True

# convenience function to link request pads
def link_request_pads(el1, tpl1, el2, tpl2):
    pad1 = el1.request_pad(el1.get_pad_template(tpl1), None, None)
    pad2 = el2.request_pad(el2.get_pad_template(tpl2), None, None)
    pad1.link(pad2)
    return pad2


# a TS demuxer pad has been added
def on_pad_added(src, pad, *user_data):

    global new_client

    name = pad.get_name()
    print("tsdemux pad added: "+name)

    # find corresponding ssrc for the active demuxer
    ssrc = src.get_name().split("_")[-1]
    teename = "tee_"+ssrc+"_"+stream[name]

    # create video decoders
    if name.startswith("video"):

        clients[ssrc].create_video_decoder(src,teename)

    # create audio decoder
    if name.startswith("audio"):

        clients[ssrc].create_audio_decoder(src,teename)

        # audio stream is last one in bundle, so if this pad has been added,
        # video streams are complete as well -> check mixer links in idle func
        new_client.append(ssrc)
        GLib.timeout_add(1000, mixer_check_cb, None)


# create single mixer for front stream
def create_frontmixer_queue():

    global frontmixer
    global frontstream

    if frontmixer != None:
        return

    print("  creating frontmixer subqueue")

    frontmixer = new_element("compositor",myname="frontmixer")
    frontstream = new_element("tee",{"allow-not-linked":True},myname="frontstream")

    add_and_link([
        frontmixer,
        new_element("videoconvert"),
        new_element("queue",{"max-size-buffers":1}),
        new_element("x264enc",x264params),
        frontstream
    ])


def mixer_check_cb(*user_data):

    global new_client

    if len(new_client) > 0 and len(clients) >= 2:

        print("new client(s), checking mixer links... ",end="")

        # only try to link things when all clients have all stream decoders in place
        for c in clients:
            if not clients[c].ready():
                print("not all decoders in place yet, waiting.")
                return True #GLib.SOURCE_CONTINUE

        print("all client decoders ready.")

        create_frontmixer_queue()

        ssrc = new_client.pop(0)
        print("  setting up mixers for new client "+ssrc)

        # create surface mixers for _all_ clients (where still needed)
        for c in clients:
            clients[c].create_surface_mixer()
            clients[c].create_audio_mixer()

        # add missing frontmixer links
        clients[ssrc].link_to_front()

        # add missing surface mixer links
        clients[ssrc].link_surface_streams()

        # add missing audio stream links
        clients[ssrc].link_audio_streams()

        # write out debug dot file (needs envvar GST_DEBUG_DUMP_DOT_DIR set)
        Gst.debug_bin_to_dot_file(pipeline,Gst.DebugGraphDetails(15),"debug")

    return len(new_client) > 0 # GLib.SOURCE_CONTINUE/_REMOVE


# new ssrc coming in on UDP socket, i.e. new client
def on_ssrc_pad(src, pad, *user_data):

    global clients

    name = pad.get_name()
    ssrc = name.split("_")[-1]
    jbname = "rtpjb_"+ssrc
    print("ssrc pad added: "+name)

    # TODO: maybe use a specific fixed SSRC (e.g. 0x12345678) to exit/restart?

    if name.startswith("rtcp"):
        # link the rtcp_src pad to jitterbuffer
        jb = pipeline.get_by_name(jbname)
        sinkpad = jb.request_pad(jb.get_pad_template("sink_rtcp"), None, None)
        pad.link(sinkpad)
        return

    # add pad probe for buffer metadata (which contains sender IP address)
    pad.add_probe(Gst.PadProbeType.BUFFER, probe_callback, None)

    # TODO: lower latency parameter?
    tsdemux = new_element("tsdemux",myname="tsd_"+ssrc)
    tsdemux.connect("pad-added",on_pad_added)

    add_and_link([ src,
        new_element("rtpjitterbuffer", { "do-lost": True }, jbname ),
        new_element("rtpmp2tdepay"),
        tsdemux
    ])

    # store client metadata
    clients[ssrc] = Client(ssrc)


# pad probe for reading address metadata
def probe_callback(pad,info,pdata):
    buf = info.get_buffer()
    foo = GstNet.buffer_get_net_address_meta(buf)
    addr = foo.addr.get_address().to_string() #+":"+str(foo.addr.get_port())
    ssrc = pad.get_name().split("_")[-1]
    if ssrc in clients:
        clients[ssrc].ip = addr
    return Gst.PadProbeReturn.OK


def main(args):

    global pipeline

    print("SurfaceCast stream mixer component 0.1")

    Gst.init(None)

    pipeline = Gst.Pipeline()

    caps = Gst.Caps.from_string("application/x-rtp,media=video,clock-rate=90000,encoding-name=MP2T")

    # setup demuxer with callback for new ssrc
    rtpdemux = new_element("rtpssrcdemux")
    rtpdemux.connect("pad-added",on_ssrc_pad)

    # setup udp src with pad probe to retrieve address metadata
    # TODO: make port configurable
    udpsrc = new_element("udpsrc", { "port": 5000, "retrieve-sender-address": True } )

    # initial pipeline: udpsrc -> caps -> rtpdemux
    add_and_link([ udpsrc, new_element("capsfilter", { "caps": caps } ), rtpdemux ])

    # kick things off
    pipeline.set_state(Gst.State.PLAYING)
    loop = GLib.MainLoop()

    # listen for bus messages
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect ("message", bus_call, loop)

    try:
        loop.run()
    except:
        pass

    pipeline.set_state(Gst.State.NULL)

sys.exit(main(sys.argv))

