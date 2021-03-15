#include <unistd.h>
#include <iostream>

#include "V4L2.h"

#define IN_W 1280
#define IN_H  720
#define IN_F   15

V4L2::V4L2(const char* pipe, int devnum):
  Camera(pipe,"BGR",IN_W,IN_H), 
  cap(devnum,cv::CAP_V4L2)
{
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    throw std::runtime_error("Unable to open V4L2 camera device.");

  cap.set(cv::CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(cv::CAP_PROP_FPS,IN_F);
  cap.set(cv::CAP_PROP_FOURCC,cv::VideoWriter::fourcc('M','J','P','G'));
}

void V4L2::retrieve_frames() {
  cap >> input;
}
