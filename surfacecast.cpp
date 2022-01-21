#include <unistd.h>
#include <iostream>

#include "V4L2.h"
#include "SUR40.h"
#include "VirtualCam.h"

#ifdef REALSENSE
  #include "Realsense.h"
#endif

#ifdef K4A
  #include "KinectAzure.h"
#endif

#define IN_W 1280
#define IN_H  720

int get_v4l_devnum(const char* path) {
  char buf[128]; if (!path) path = "/dev/video0";
  int num,res = readlink(path,buf,sizeof(buf));
  if (res == -1) { strncpy(buf,path,sizeof(buf)); res = strlen(buf); }
  num = (int)(buf[res-1] - '0');
  std::cout << "path " << path << " maps to " << buf << ", devnum " << num << std::endl;
  return num;
}

int main(int argc, char* argv[]) {

  std::cout << "\nSurfaceCast v0.2.0 - https://github.com/floe/surfacecast\n" << std::endl;

  if (argc < 3) {
    std::cout << "usage: surfacecast <camtype> <videodev> [\"gstreamer_pipeline\"]\n" << std::endl;
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

  char* gstpipe = nullptr;
  if (argc > 3) gstpipe = argv[3];

  Camera* cam = nullptr;

  if (std::string(argv[1]) ==    "v4l2") cam = new V4L2(gstpipe,get_v4l_devnum(argv[2]),IN_W,IN_H);
  if (std::string(argv[1]) ==   "sur40") cam = new SUR40(gstpipe,get_v4l_devnum(argv[2]));
  if (std::string(argv[1]) == "virtcam") cam = new VirtualCam(gstpipe);

#ifdef REALSENSE
  if (std::string(argv[1]) == "realsense") cam = new Realsense(gstpipe);
#endif

#ifdef K4A
  if (std::string(argv[1]) == "k4a") cam = new KinectAzure(gstpipe);
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
