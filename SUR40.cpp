#include "SUR40.h"

#define SUR40_W 960
#define SUR40_H 540

SUR40::SUR40(const char* pipe, int devnum): V4L2(pipe,devnum,SUR40_W,SUR40_H) {
  cap.set(cv::CAP_PROP_CONVERT_RGB, false);
}

void SUR40::remove_background() {

  cv::Mat output(SUR40_H,SUR40_W,CV_8UC3);
  uint8_t* in_data = (uint8_t*)input.data;
  uint8_t* out_data = (uint8_t*)output.data;
  for (int i = 0; i < SUR40_W*SUR40_H; i++) {
    uint8_t val = in_data[i];
    if (val < 128) { // FIXME: hardcoded threshold
      out_data[i*3+0] = 0;
      out_data[i*3+1] = 0xFF;
      out_data[i*3+2] = 0;
    } else {
      out_data[i*3+0] = val;
      out_data[i*3+1] = val;
      out_data[i*3+2] = val;
    }
  }
  
  input = output;
}
