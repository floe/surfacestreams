#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>

#include "V4L2.h"
#include "SUR40.h"
#include "VirtualCam.h"

#ifdef REALSENSE
  #include "Realsense.h"
#endif

#ifdef K4A
  #include "KinectAzure.h"
#endif

int main(int argc, char* argv[]) {

  int in_w = 1280, in_h = 720;
  std::vector<std::string> args( argv+1, argv+argc );

  std::cout << "\nSurfaceCast v0.3.0 - https://github.com/floe/surfacecast\n" << std::endl;

  std::string camtype = "";
  const char* device = "0";
  const char* gstpipe = 0;

  for (auto arg = args.begin(); arg != args.end(); arg++) {
    if (*arg == "-f") { do_filter = false;          continue; }
    if (*arg == "-b") { do_blank  = true;           continue; }
    if (*arg == "-t") { camtype = *(++arg);         continue; }
    if (*arg == "-d") { device  = (++arg)->c_str(); continue; }
    if (*arg == "-p") { gstpipe = (++arg)->c_str(); continue; }
    if (*arg == "-s") {
      std::string size = *(++arg);
      int x = size.find("x");
      in_w = stoi(size.substr(  0, x));
      in_h = stoi(size.substr(x+1,-1));
    }
  }

  if (camtype == "") {
    std::cout << "usage: surfacecast -t <camtype> -d <videodev> [-b] [-f] [-p \"gstreamer_pipeline\"] [-s wxh]\n" << std::endl;
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

  Camera* cam = nullptr;

  if (camtype ==    "v4l2") cam = new V4L2(gstpipe,device,in_w,in_h);
  if (camtype ==   "sur40") cam = new SUR40(gstpipe,device);
  if (camtype == "virtcam") cam = new VirtualCam(gstpipe);

#ifdef REALSENSE
  if (camtype == "realsense") cam = new Realsense(gstpipe);
#endif

#ifdef K4A
  if (camtype == "k4a") cam = new KinectAzure(gstpipe);
#endif

  while (!cam->do_quit) {

    cam->retrieve_frames();

    if (cam->find_plane) {
      cam->ransac_plane();
      cam->find_plane = false;
    }

    cam->remove_background();

    cam->send_buffer();
  }

  return 0;
}
