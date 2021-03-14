#include "SUR40.h"

#define SUR40_W 960
#define SUR40_H 540

SUR40::SUR40(const char* pipe): Camera(pipe,"BGR",SUR40_W,SUR40_H) { }

void SUR40::process_frames(cv::Mat* color, cv::Mat* depth) {

  cv::Mat output(SUR40_W,SUR40_H,CV_8UC3);
  uint8_t* in_data = (uint8_t*)color->data;
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
  
  *color = output;
}
