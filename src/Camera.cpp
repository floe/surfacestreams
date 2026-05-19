#include "Camera.h"

#include <vector>
#include <iostream>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////
//
// OpenCV stuff
//

#include <opencv2/core/core.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>

using namespace cv;

// yuck, globals
bool do_blank = false;
bool do_filter = true;
void usr1handler(int signal) { do_blank = !do_blank; }
void usr2handler(int signal) { do_filter = !do_filter; }

Camera::Camera(const char* _pipe, const char* _type, int _cw, int _ch, int _tw, int _th ): calib(_cw,_ch,_tw,_th), overlay(_tw,_th) {
  cw = _cw; ch = _ch; 
  tw = _tw; th = _th;

  signal(SIGUSR1,&usr1handler);
  signal(SIGUSR2,&usr2handler);

  loadConfig();
  gstreamer_init(_type,_pipe);

  find_plane = false;
  autocalib = false;
  do_quit = false;
}

cv::FileStorage Camera::loadConfig() {
  cv::FileStorage file("config.xml", cv::FileStorage::READ);
  if (!file.isOpened()) {
    calib.reset();
  } else {
    file["perspective"] >> calib.pm;
  }
  return file;
}

cv::FileStorage Camera::saveConfig() {
  cv::FileStorage file("config.xml", cv::FileStorage::WRITE);
  file << "perspective" << calib.pm;
  std::cout << "configuration saved to config.xml" << std::endl;
  return file;
}

void Camera::autoPerspective() { autocalib = calib.autoPerspective(input); }
void Camera::ransac_plane() { } // unimplemented, only makes sense for DepthCamera

////////////////////////////////////////////////////////////////////////////////
//
// GStreamer stuff
//

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/navigation.h>

#include <stdint.h>
#include <string.h>

gboolean pad_event(GstPad *pad, GstObject *parent, GstEvent *event) {

  if (GST_EVENT_TYPE (event) != GST_EVENT_NAVIGATION)
    return gst_pad_event_default(pad,parent,event);

  double x,y; int b;
  const gchar* key;
  Camera* cam = (Camera*)gst_pad_get_element_private(pad);

  switch (gst_navigation_event_get_type(event)) {

    // calibration: top (left, right), bottom (left, right)
    case GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE:
      gst_navigation_event_parse_mouse_button_event(event,&b,&x,&y);
      cam->calib.push_point(x,y);
      break;

    case GST_NAVIGATION_EVENT_KEY_PRESS:
      gst_navigation_event_parse_key_event(event,&key);
      cam->handle_key(key);
      break;

    default:
      return false;
  }

  return true;
}

void Camera::handle_key(const char* key) {

      // reset perspective matrix
      if (key == std::string("space"))
        calib.reset();

      // autocalibrate
      if (key == std::string("a"))
        autocalib = true;

      // find largest plane
      if (key == std::string("p"))
        find_plane = true;

      // subtract plane
      if (key == std::string("f"))
        do_filter = !do_filter;

      // blank
      if (key == std::string("b"))
        do_blank = !do_blank;

      // quit
      if (key == std::string("q"))
        do_quit = true;

      // save
      if (key == std::string("s"))
        saveConfig();

      // change plane distance threshold
      /*if (key == std::string( "plus")) distance += 0.2;
      if (key == std::string("minus")) distance -= 0.2;

      std::cout << "current plane distance: " << distance << " cm " << std::endl;*/
}

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

  // attach event listener to suitable src pad (either appsrc or videoconvert)
  // FIXME: ugly hack, hardcoded element name
  GstElement* display = gst_bin_get_by_name( (GstBin*)gpipeline, "display" );
  GstPad* srcpad = gst_element_get_static_pad (display?display:appsrc, "src");
  gst_pad_set_event_function( srcpad, (GstPadEventFunction)pad_event );
  gst_pad_set_element_private( srcpad, (gpointer)this);

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

void buffer_destroy(gpointer data) {
  cv::Mat* done = (cv::Mat*)data;
  delete done;
}

void Camera::send_buffer() {

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

void* thread_helper(void* arg) {
	thread_info* ti = (thread_info*)arg;
	ti->obj->remove_background(ti->start,ti->end);
	return nullptr;
}

void Camera::retrieve_frames() {
  // TODO: override this in any camera subclass
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
