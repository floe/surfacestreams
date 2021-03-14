#include <unistd.h>
#include <iostream>

#include "Camera.h"
#include "SUR40.h"
#include <opencv2/videoio.hpp>

#define IN_W 1280
#define IN_H  720
#define IN_F   15

int get_v4l_devnum(const char* path) {
  char buf[128]; if (!path) path = "/dev/video0";
  int num,res = readlink(path,buf,sizeof(buf));
  if (res == -1) { strncpy(buf,path,sizeof(buf)); res = strlen(buf); }
  num = (int)(buf[res-1] - '0');
  std::cout << "path " << path << " maps to " << buf << ", devnum " << num << std::endl;
  return num;
}

int main(int argc, char* argv[]) {

  char* gstpipe = nullptr;
  if (argc > 2) gstpipe = argv[2];

  #ifndef SUR40
  Camera cam(gstpipe,"BGR",IN_W,IN_H);
  #else
  SUR40 cam(gstpipe);
  #endif

  cv::VideoCapture cap(get_v4l_devnum(argv[1]),cv::CAP_V4L2);
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    return 1;

  cap.set(cv::CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(cv::CAP_PROP_FPS,IN_F);
  cap.set(cv::CAP_PROP_FOURCC,cv::VideoWriter::fourcc('M','J','P','G'));

  while (!cam.do_quit) {

    cv::Mat input;
    cap >> input;

    cam.process_frames(&input);

    cam.prepare_buffer(&input,input.type());
  }

  return 0;
}
