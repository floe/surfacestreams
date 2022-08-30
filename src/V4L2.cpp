#include <unistd.h>
#include <iostream>

#include "V4L2.h"

V4L2::V4L2(const char* pipe, const char* dev, int cw, int ch):
  Camera(pipe,"BGR",cw,ch),
  cap(std::string(dev),cv::CAP_V4L2)
{
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    throw std::runtime_error("Unable to open V4L2 camera device.");

  cap.set(cv::CAP_PROP_FRAME_WIDTH,cw);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT,ch);
  cap.set(cv::CAP_PROP_FPS,15); // FIXME: hardcoded FPS
  cap.set(cv::CAP_PROP_FOURCC,cv::VideoWriter::fourcc('Y','U','Y','V'));
}

void V4L2::retrieve_frames() {
  cap >> input;
}

// courtesy of https://www.geeksforgeeks.org/program-change-rgb-color-model-hsv-color-model/
void BGRtoHSV(uint8_t b, uint8_t g, uint8_t r, double* out) {

  double cmax = std::max(r, std::max(g, b));
  double cmin = std::min(r, std::min(g, b));
  double diff = cmax - cmin;

  double s = (cmax == 0) ? 0.0 : (diff / cmax);
  double v = cmax / 255.0;

  out[1] = s;
  out[2] = v;
}

void V4L2::remove_background() {

  uint8_t* in_data = (uint8_t*)input.data;

  if (do_filter && in_data) for (int i = 0; i < input.rows*input.cols*3; i+=3) {

    uint8_t r = in_data[i+0];
    uint8_t g = in_data[i+1];
    uint8_t b = in_data[i+2];

    double hsv[3]; BGRtoHSV(b,g,r,hsv);

    // FIXME: hardcoded threshold
    if (hsv[1] < 0.3 && hsv[2] > 0.7) {
      in_data[i+0] = 0;
      in_data[i+1] = 0xFF;
      in_data[i+2] = 0;
    }
  }
}
