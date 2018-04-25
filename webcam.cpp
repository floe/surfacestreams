#include "common.h"

void buffer_destroy(gpointer data) {
  cv::Mat* done = (cv::Mat*)data;
  delete done;
}

GstFlowReturn prepare_buffer(GstAppSrc* appsrc, cv::Mat* frame) {

  guint size = 1280 * 720 * 4;
  GstBuffer *buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, (gpointer)(frame->data), size, 0, size, frame, buffer_destroy );

  return gst_app_src_push_buffer(appsrc, buffer);
}

#ifdef GSTREAMER_LIBFREENECT2

void buffer_destroy(gpointer data) {
  libfreenect2::Frame* done = (libfreenect2::Frame*)data;
  delete done;
}

GstFlowReturn prepare_buffer(GstAppSrc* appsrc, libfreenect2::Frame* frame) {

  guint size = 1280 * 720 * 4;
  GstBuffer *buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, (gpointer)(frame->data), size, 0, size, frame, buffer_destroy );

  return gst_app_src_push_buffer(appsrc, buffer);
}

#endif

#define IN_W 1280
#define IN_H  720
#define IN_F   15

int get_v4l_devnum(const char* path) {
  char buf[128]; if (!path) path = "";
  int num,res = readlink(path,buf,sizeof(buf));
  num = (res == -1) ? 0 : (int)(buf[res-1] - '0');
  std::cout << "path " << (path?path:"NULL") << " maps to devnum " << num << std::endl;
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

    prepare_buffer((GstAppSrc*)appsrc,output);
    g_main_context_iteration(g_main_context_default(),FALSE);
  }

  /* clean up */
  gst_element_set_state (gpipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (gpipeline));

  return 0;
}
