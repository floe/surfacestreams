#include "common.h"

#define IN_W 1280
#define IN_H  720
#define IN_F   15

int get_v4l_devnum(const char* path) {
  char buf[128]; if (!path) path = "";
  int num,res = readlink(path,buf,sizeof(buf));
  if (res == -1) { strncpy(buf,path,sizeof(buf)); res = strlen(buf); }
  num = (int)(buf[res-1] - '0');
  std::cout << "path " << path << " maps to " << buf << ", devnum " << num << std::endl;
  return num;
}

int main(int argc, char* argv[]) {

  bool _quit = false; quit = &_quit;
  if (argc > 2) gstpipe = argv[2];

  opencv_init(-1,-1,IN_W,IN_H);
  gstreamer_init(argc,argv,"BGR");
  //gstreamer_init(argc,argv,"GRAY8");

  init_objects({ "template.jpg" } );

  cv::VideoCapture cap(get_v4l_devnum(argv[1]));
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    return 1;

  cap.set(CV_CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(CV_CAP_PROP_FPS,IN_F);

  while (!_quit) {

    Mat input;
    cap >> input;

    track_objects(input);

    #ifdef SUR40
    cap.set(CV_CAP_PROP_CONVERT_RGB, false);
    Mat output(540,960,CV_8UC3);
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

    prepare_buffer(&input,IN_W,IN_H,input.type());
  }

  gstreamer_cleanup();

  return 0;
}
