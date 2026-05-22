#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>

#include "V4L2.h"
#include "SUR40.h"
#include "VirtualCam.h"

#include <gst/gst.h>
#include <gst/video/navigation.h>

#ifdef REALSENSE
  #include "Realsense.h"
#endif

#ifdef K4A
  #include "KinectAzure.h"
#endif

bool do_quit = false;
bool do_blank = false;
bool do_filter = true;
bool find_plane = false;
bool do_undistort = false;

void usr1handler(int signal) { do_blank = !do_blank; }
void usr2handler(int signal) { do_filter = !do_filter; }

Camera* cam = nullptr;

void handle_key(const char* key) {

  // reset perspective matrix
  if (key == std::string("space"))
    cam->calib.reset();

  // autocalibrate
  if (key == std::string("a"))
    cam->autocalib = true;

  // find largest plane
  if (key == std::string("p"))
    find_plane = true;

  // subtract plane
  if (key == std::string("f"))
    do_filter = !do_filter;

  // blank
  if (key == std::string("b"))
    do_blank = !do_blank;

  // quit
  if (key == std::string("q"))
    do_quit = true;

  // save
  if (key == std::string("s"))
    cam->saveConfig();

  // undistort
  if (key == std::string("u"))
    do_undistort = !do_undistort;

  // change plane distance threshold
  /*if (key == std::string( "plus")) distance += 0.2;
  if (key == std::string("minus")) distance -= 0.2;

  std::cout << "current plane distance: " << distance << " cm " << std::endl;*/
}

gboolean pad_event(GstPad *pad, GstObject *parent, GstEvent *event) {

  if (GST_EVENT_TYPE (event) != GST_EVENT_NAVIGATION)
    return gst_pad_event_default(pad,parent,event);

  double x,y; int b;
  const gchar* key;
  //Camera* cam = (Camera*)gst_pad_get_element_private(pad);

  switch (gst_navigation_event_get_type(event)) {

    // calibration: top (left, right), bottom (left, right)
    case GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE:
      gst_navigation_event_parse_mouse_button_event(event,&b,&x,&y);
      cam->calib.push_point(x,y);
      break;

    case GST_NAVIGATION_EVENT_KEY_PRESS:
      gst_navigation_event_parse_key_event(event,&key);
      handle_key(key);
      break;

    default:
      return false;
  }

  return true;
}

int main(int argc, char* argv[]) {

  signal(SIGUSR1,&usr1handler);
  signal(SIGUSR2,&usr2handler);

  int  in_w = 1280,  in_h = 720;
  int out_w = 1280, out_h = 720;
  std::vector<std::string> args( argv+1, argv+argc );

  std::cout << "\nSurfaceCast v0.3.0 - https://github.com/floe/surfacecast\n" << std::endl;

  std::string camtype = "";
  const char* device = "0";
  const char* gstpipe = 0;

  // some argument parsing
  for (auto arg = args.begin(); arg != args.end(); arg++) {
    if (*arg == "-f") { do_filter = false;          continue; }
    if (*arg == "-b") { do_blank  = true;           continue; }
    if (*arg == "-t") { camtype = *(++arg);         continue; }
    if (*arg == "-d") { device  = (++arg)->c_str(); continue; }
    if (*arg == "-p") { gstpipe = (++arg)->c_str(); continue; }
    if (*arg == "-is") {
      std::string size = *(++arg);
      int x = size.find("x");
      in_w = stoi(size.substr(  0, x));
      in_h = stoi(size.substr(x+1,-1));
      continue;
    }
    if (*arg == "-os") {
      std::string size = *(++arg);
      int x = size.find("x");
      out_w = stoi(size.substr(  0, x));
      out_h = stoi(size.substr(x+1,-1));
      continue;
    }
  }

  if (camtype == "") {
    std::cout << "usage: surfacecast -t <camtype> -d <videodev> [-b] [-f] [-p \"gstreamer_pipeline\"] [-is WxH] [-os WxH]\n" << std::endl;
    std::cout << "available camera types:\n" << std::endl;
    std::cout << "       v4l2 </dev/videoX> - standard V4L2 device (webcam)" << std::endl;
    std::cout << "      sur40 </dev/videoX> - SUR40 video device" << std::endl;
    std::cout << "    virtcam     <ignored> - virtual camera device" << std::endl;
#ifdef REALSENSE
    // FIXME: devnum not implemented yet
    std::cout << "  realsense      <devnum> - Intel RealSense depth camera" << std::endl;
#endif
#ifdef K4A
    std::cout << "        k4a      <devnum> - Kinect Azure depth camera" << std::endl;
#endif
    std::cout << std::endl;
    return 1;
  }

  std::cout << "Opening device " << device << " as camera type " << camtype << " with size " << in_w << "x" << in_h << "..." << std::endl;

  if (camtype ==    "v4l2") cam = new V4L2(gstpipe,device,in_w,in_h);
  if (camtype ==   "sur40") cam = new SUR40(gstpipe,device);
  if (camtype == "virtcam") cam = new VirtualCam(gstpipe);

#ifdef REALSENSE
  if (camtype == "realsense") cam = new Realsense(gstpipe);
#endif

#ifdef K4A
  if (camtype == "k4a") cam = new KinectAzure(gstpipe);
#endif

  cam->setPadHandler(pad_event);

  while (!do_quit) {

    cam->retrieve_frames();

    if (do_undistort) cam->undistort();
    if (find_plane) cam->ransac_plane();
    if (cam->autocalib) cam->autoPerspective();

    cam->remove_background();

    cam->send_buffer();
  }

  return 0;
}
