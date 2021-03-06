#!/usr/bin/python3

import sys,gi,json,argparse,datetime
gi.require_version('GLib', '2.0')
gi.require_version('Gst',  '1.0')
gi.require_version('Soup', '2.4')
gi.require_version('GstWebRTC', '1.0')
gi.require_version('GstSdp', '1.0')
from gi.repository import GLib, Gst, Soup, GstWebRTC, GstSdp

from gst_helpers import *
from webrtc_peer import *
from client import *

args = None

# get address and port from client
def get_client_address(client):
    addr = client.get_remote_address()
    return addr.get_address().to_string()+"_"+str(addr.get_port())

# incoming HTTP(S) request
def http_handler(server,msg,path,query,client,user_data):
    logging.info("HTTP(S) request for: "+path)
    #flags[get_client_address(client)] = query
    content_type = "text/html"
    try:
        data = open("webclient"+path,"r").read()
        if path.endswith(".js"):
            content_type = "text/javascript"
        msg.set_status(Soup.Status.OK)
    except:
        msg.set_status(Soup.Status.NOT_FOUND)
        data=path+" not found"
        if path == "/quit":
            logging.info("Well... bye.")
            GLib.timeout_add(100,quit_mainloop)
            data="Server exiting/restarting..."

    msg.response_headers.append("Content-Type",content_type)
    msg.response_headers.append("Cache-Control","no-store")
    msg.response_body.append(data.encode("utf-8"))

# Websocket connection was closed by remote
def ws_close_handler(connection, client):
    logging.info("WebSocket closed by remote.")
    client.remove()

# incoming Websocket connection
def ws_conn_handler(server, connection, path, client, user_data):

    source = get_client_address(client)
    logging.info("New WebSocket connection from: "+source)

    wrb = WebRTCPeer(connection,source,args.stun)
    new_client = Client(source,wrb)
    connection.connect("closed",ws_close_handler,new_client)

# "main"
print("\nSurfaceStreams backend mixing server v0.1.0 - https://github.com/floe/surfacestreams\n")
print("Note: any GStreamer-WARNINGs about pipeline loops can be safely ignored.\n")

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
outfile = datetime.datetime.now().strftime("surfacestreams-%Y%m%d-%H%M%S.mp4")

parser.add_argument("-d","--debug", help="more debug output (-dd=max)",  action="count",default=1 )
parser.add_argument("-s","--sink",  help="save all streams to MP4 file", action="store_true"      )
parser.add_argument("-p","--port",  help="server HTTPS listening port",  default=8080             )
parser.add_argument("-o","--out",   help="MP4 output filename", default=outfile                   )
parser.add_argument("-u","--stun",  help="STUN server", default="stun://stun.l.google.com:19302"  )

args = parser.parse_args()
print("Option",args,"\n")

init_pipeline(on_element_added,args.debug)

frontsrc   = "filesrc location=assets/front.png ! pngdec ! videoconvert ! imagefreeze ! identity sync=true"
surfacesrc = "videotestsrc is-live=true pattern=solid-color foreground-color=0" #ball motion=sweep background-color=0
audiosrc   = "audiotestsrc is-live=true wave=silence"

add_test_sources(frontsrc,surfacesrc,audiosrc,fake=True,bgcol=0xFFFF00FF,wave="sine")
create_frontmixer_queue()

server = Soup.Server()
server.add_handler("/",http_handler,None)
server.add_websocket_handler("/ws",None,None,ws_conn_handler,None)
server.set_ssl_cert_file("assets/tls-cert.pem","assets/tls-key.pem")
server.listen_all(int(args.port),Soup.ServerListenOptions.HTTPS)

if args.sink:
    logging.info("Adding file sink client...")
    sink = StreamSink("file_sink",args.out)
    client = Client("file_sink",sink)
    client.link_new_client()

run_mainloop()
