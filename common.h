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

using namespace cv;

void opencv_init();

////////////////////////////////////////////////////////////////////////////////
//
// plane model stuff
//

#include <Eigen/Core>
#include <SimpleRansac.h>
#include <PlaneModel.h>

extern PlaneModel<float> plane;

////////////////////////////////////////////////////////////////////////////////
//
// GStreamer stuff
//

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/navigation.h>

extern bool find_plane;
extern bool filter;
extern bool *quit;

extern float distance;

extern char* gstpipe;

extern Mat pm;
extern GstElement* appsrc;
extern GstElement* gpipeline;

void gstreamer_init(gint argc, gchar *argv[], const char* type);

