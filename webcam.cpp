#include "common.h"

void buffer_destroy(gpointer data) {
  cv::Mat* done = (cv::Mat*)data;
  delete done;
}


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

  opencv_init();
  gstreamer_init(argc,argv,"BGR");

  cv::VideoCapture cap(get_v4l_devnum(argv[1]));
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    return 1;

  //cv::VideoWriter video;

  cap.set(CV_CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(CV_CAP_PROP_FPS,IN_F);
  Mat* output;

  while (!_quit) {

    Mat input;
    cap >> input;

    output = new Mat(720,1280,CV_8UC3);
    warpPerspective(input,*output,pm,output->size(),INTER_NEAREST);

    prepare_buffer(IN_W*IN_H*4,output->data,output,buffer_destroy);
    g_main_context_iteration(g_main_context_default(),FALSE);
  }

  gstreamer_cleanup();

  return 0;
}
