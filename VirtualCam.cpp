#include "VirtualCam.h"

#include <unistd.h>
#include <iostream>
#include <opencv2/imgproc.hpp>

VirtualCam::VirtualCam(const char* pipe):
  Camera( pipe, "BGRA", tw, th )
{
  input = cv::Mat( th, tw, CV_8UC4, cv::Scalar(0,255,0,0) );
  pm = im;
}

void VirtualCam::retrieve_frames() {
  usleep(95000);
}

void VirtualCam::push_point(float x, float y) {
  // TODO: make continuous while button pushed
  // TODO: clear again when right button down
  cv::RotatedRect rr(cv::Point2f(x,y), cv::Size2f(20,20), 0);
  cv::ellipse(input,rr,cv::Scalar(255,255,255,255),-1);
}

//void VirtualCam::handle_key(const char* key) { }
