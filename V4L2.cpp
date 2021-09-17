#include <unistd.h>
#include <iostream>

#include "V4L2.h"

V4L2::V4L2(const char* pipe, int devnum, int cw, int ch):
  Camera(pipe,"BGR",cw,ch),
  cap(devnum,cv::CAP_V4L2)
{
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    throw std::runtime_error("Unable to open V4L2 camera device.");

  cap.set(cv::CAP_PROP_FRAME_WIDTH,cw);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT,ch);
  cap.set(cv::CAP_PROP_FPS,15); // FIXME: hardcoded FPS
  cap.set(cv::CAP_PROP_FOURCC,cv::VideoWriter::fourcc('M','J','P','G'));
}

void V4L2::retrieve_frames() {
  cap >> input;
}
