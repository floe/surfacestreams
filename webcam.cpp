#include "common.h"
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>

#define IN_W 1280
#define IN_H  720
#define IN_F   10

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

  cv::VideoCapture cap(get_v4l_devnum(argv[1]));
  if (!cap.isOpened())  // check if succeeded to connect to the camera
    return 1;

  cap.set(CV_CAP_PROP_FRAME_WIDTH,IN_W);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT,IN_H);
  cap.set(CV_CAP_PROP_FPS,IN_F);

  #ifdef BGSUB
    CascadeClassifier face;
    face.load("/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml");
    Ptr<BackgroundSubtractor> bgsub = createBackgroundSubtractorMOG2();
    Mat mask(IN_H,IN_W,CV_8UC1);
    Mat mask_tmp(IN_H,IN_W,CV_8UC1);
    Mat element = getStructuringElement( MORPH_RECT, Size( 3, 3 ) );
  #endif

  while (!_quit) {

    Mat input;
    cap >> input;

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

    #ifdef BGSUB
    bgsub->apply(input,mask_tmp);
    erode(mask_tmp,mask,element);
    
    Mat frame_gray;
    cvtColor(input,frame_gray,COLOR_BGR2GRAY);
    std::vector<Rect> faces;
    face.detectMultiScale(frame_gray,faces);
    for (size_t i = 0; i < faces.size(); i++ ) {
      Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
      ellipse( mask, center, Size( faces[i].width/1.5, faces[i].height/1.5 ), 0, 0, 360, Scalar( 255, 255, 255 ), -1 );
    }

    std::vector< std::vector<Point> > contours;
    std::vector<Vec4i> hierarchy;
    findContours(mask,contours,hierarchy,RETR_CCOMP,CHAIN_APPROX_SIMPLE);
    for(int idx = 0 ; idx >= 0; idx = hierarchy[idx][0] )
    {
        Scalar color( rand()&255, rand()&255, rand()&255 );
        drawContours( input, contours, idx, color, FILLED, 8, hierarchy );
    }

/*    Mat output(IN_H,IN_W,CV_8UC3);
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
    input = output;*/
    #endif

    prepare_buffer(&input,IN_W,IN_H,input.type());
  }

  gstreamer_cleanup();

  return 0;
}
