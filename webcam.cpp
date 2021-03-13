#include <unistd.h>
#include <iostream>

#include "Camera.h"
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

  Camera cam(gstpipe,"BGR",IN_W,IN_H);

  cv::VideoCapture cap(get_v4l_devnum(argv[1]),cv::CAP_V4L2);
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    return 1;

  cap.set(cv::CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(cv::CAP_PROP_FPS,IN_F);

  #ifdef BGSUB
    Ptr<BackgroundSubtractor> bgsub = createBackgroundSubtractorMOG2();
    Mat mask(IN_H,IN_W,CV_8UC1);
  #endif

  while (!cam.do_quit) {

    cv::Mat input;
    cap >> input;

    #ifdef SUR40
    cap.set(cv::CAP_PROP_CONVERT_RGB, false);
    cv::Mat output(540,960,CV_8UC3);
    uint8_t* in_data = (uint8_t*)input.data;
    uint8_t* out_data = (uint8_t*)output.data;
    for (int i = 0; i < 960*540; i++) {
      uint8_t val = in_data[i];
      if (val < 128) {
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
    #endif

    #ifdef BGSUB
    bgsub->apply(input,mask);
    Mat output(IN_H,IN_W,CV_8UC3);
    uint8_t* in_data = (uint8_t*)input.data;
    uint8_t* out_data = (uint8_t*)output.data;
    uint8_t* mask_data = (uint8_t*)mask.data;
    for (int i = 0; i < IN_W*IN_H; i++) {
      uint8_t val = mask_data[i];
      if (val < 128) {
        out_data[i*3+0] = 0;
        out_data[i*3+1] = 0xFF;
        out_data[i*3+2] = 0;
      } else {
        out_data[i*3+0] = in_data[i*3+0];
        out_data[i*3+1] = in_data[i*3+1];
        out_data[i*3+2] = in_data[i*3+2];
      }
    }
    input = output;
    #endif

    cam.prepare_buffer(&input,IN_W,IN_H,input.type());
  }

  return 0;
}
