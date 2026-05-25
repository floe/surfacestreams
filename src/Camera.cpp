#include "Camera.h"

#include <vector>
#include <iostream>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////
//
// OpenCV stuff
//

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d.hpp>

using namespace cv;

Camera::Camera(const char* _pipe, const char* _type, int _cw, int _ch, int _tw, int _th ): calib(_cw,_ch,_tw,_th), overlay(_tw,_th) {
  cw = _cw; ch = _ch; 
  tw = _tw; th = _th;

  loadConfig();
  gstreamer_init(_type,_pipe);

  // init undistortion maps
  Mat newCam = getOptimalNewCameraMatrix( camMat, distCoeffs, Size(cw,ch), 0.5 );
  initUndistortRectifyMap( camMat, distCoeffs, Mat(), newCam, Size(cw,ch), CV_16SC2, map1, map2);
}

cv::FileStorage Camera::loadConfig() {
  cv::FileStorage file("config.xml", cv::FileStorage::READ);
  if (!file.isOpened()) {
    calib.reset();
    camMat = Mat_<float>({3, 3}, {1, 0, cw/2.0f, 0, 1, ch/2.0f, 0, 0, 1});
    distCoeffs = Mat_<float>({5,1}, {0, 0, 0, 0, 0});
  } else {
    file["perspective"] >> calib.pm;
    file["camMat"] >> camMat;
    file["distCoeffs"] >> distCoeffs;
  }
  return file;
}

cv::FileStorage Camera::saveConfig() {
  cv::FileStorage file("config.xml", cv::FileStorage::WRITE);
  file << "perspective" << calib.pm;
  file << "camMat" << camMat;
  file << "distCoeffs" << distCoeffs;
  std::cout << "configuration saved to config.xml" << std::endl;
  return file;
}

bool Camera::auto_perspective() { return calib.autoPerspective(input); }
void Camera::ransac_plane() { } // unimplemented, only makes sense for DepthCamera

void Camera::undistort() {
  Mat tmp;
  remap(input,tmp,map1,map2,INTER_LINEAR);
  input = tmp;
}

////////////////////////////////////////////////////////////////////////////////
//
// GStreamer stuff
//

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

void Camera::gstreamer_init(const char* type, const char* gstpipe) {

  /* init GStreamer */
  gst_init (nullptr, nullptr);

  /* setup pipeline */
  gpipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");

  // create pipeline from string
  const char* pipe_desc = gstpipe ? gstpipe : "videoconvert ! fpsdisplaysink sync=false";
  std::cout << "creating pipeline: " << pipe_desc << std::endl;
  videosink = gst_parse_bin_from_description(pipe_desc,TRUE,NULL);

  /* setup */
  g_object_set (G_OBJECT (appsrc), "caps",
    gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, type,
      "width",  G_TYPE_INT, tw,
      "height", G_TYPE_INT, th,
      "framerate", GST_TYPE_FRACTION, 1, 1,
    NULL), NULL);
  gst_bin_add_many (GST_BIN (gpipeline), appsrc, videosink, NULL);
  gst_element_link_many (appsrc, videosink, NULL);

  /* setup appsrc */
  g_object_set (G_OBJECT (appsrc),
    "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
    "format", GST_FORMAT_TIME,
    "is-live", TRUE,
    "block", TRUE,
    "do-timestamp", TRUE,
    NULL);

  /* play */
  gst_element_set_state (gpipeline, GST_STATE_PLAYING);
}

void Camera::setPadHandler( GstPadEventFunction pad_event ) {
  // attach event listener to suitable src pad (either appsrc or videoconvert)
  GstPad* srcpad = gst_element_get_static_pad( appsrc, "src" );
  gst_pad_set_event_function( srcpad, pad_event );
  gst_pad_set_element_private( srcpad, (gpointer)this);
}

void buffer_destroy(gpointer data) {
  cv::Mat* done = (cv::Mat*)data;
  delete done;
}

void Camera::send_buffer( bool do_blank ) {

  if (input.total() == 0) return;

  Mat* output = new Mat(th,tw,input.type());

  warpPerspective(input,*output,calib.pm,output->size(),INTER_LINEAR);

  if (do_blank) output->setTo(Scalar(0,255,0));

  calib.draw(output);
  overlay.process(output);

  guint size = output->total()*output->elemSize();
  gpointer data = output->data;
  void* frame = output;

  //prepare_buffer(IN_W*IN_H*4,output->data,output);

  GstBuffer *buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, data, size, 0, size, frame, buffer_destroy );

  gst_app_src_push_buffer((GstAppSrc*)appsrc, buffer); // ignoring GstFlowReturn result for now

  g_main_context_iteration(g_main_context_default(),FALSE);
}

void Camera::gstreamer_cleanup() {
  /* clean up */
  std::cout << "cleaning up" << std::endl;
  gst_element_set_state (gpipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (gpipeline));
}

////////////////////////////////////////////////////////////////////////////////
//
// thread stuff
//

typedef struct {
	Camera* obj;
	float start,end;
} thread_info;

void* thread_helper(void* arg) {
	thread_info* ti = (thread_info*)arg;
	ti->obj->remove_background(ti->start,ti->end);
	return nullptr;
}

void Camera::remove_background(float start, float end) {
  // TODO: override this in any camera subclass
}

void Camera::remove_background() {
	// TODO: hardcoded for 2 threads
	thread_info ti = { this, 0.0, 0.5 };
	pthread_t thread;
	// FIXME: threadpool would be nicer
	pthread_create(&thread,nullptr,&thread_helper,(void*)&ti);
	remove_background(0.5,1.0);
	pthread_join(thread,nullptr);
}
