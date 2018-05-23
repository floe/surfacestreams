#include "common.h"

#define IN_W 1280
#define IN_H  720
#define IN_F   15

int get_v4l_devnum(const char* path) {
  char buf[128]; if (!path) path = "";
  int num,res = readlink(path,buf,sizeof(buf));
  num = (res == -1) ? 0 : (int)(buf[res-1] - '0');
  std::cout << "path " << path << " maps to devnum " << num << std::endl;
  return num;
}

int main(int argc, char* argv[]) {

  bool _quit = false; quit = &_quit;
  if (argc > 2) gstpipe = argv[2];

  opencv_init(-1,-1,IN_W,IN_H);
  gstreamer_init(argc,argv,"BGR");

  cv::VideoCapture cap(get_v4l_devnum(argv[1]));
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    return 1;

  //cv::VideoWriter video;

  cap.set(CV_CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(CV_CAP_PROP_FPS,IN_F);

  while (!_quit) {

    Mat input;
    cap >> input;

    prepare_buffer(&input,IN_W,IN_H,CV_8UC3);
  }

  gstreamer_cleanup();

  return 0;
}
