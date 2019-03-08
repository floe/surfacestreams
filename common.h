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

extern int dw, dh, cw, ch;
void opencv_init(int dw, int dh, int cw, int ch);

void init_objects(std::vector<std::string> templates);
void track_objects(Mat foo);

////////////////////////////////////////////////////////////////////////////////
//
// plane model stuff
//

#include <Eigen/Core>
#include <SimpleRansac.h>
#include <PlaneModel.h>

extern PlaneModel<float> plane;
PlaneModel<float> ransac_plane(void (*get_3d_pt)(int,int,float*));

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

void gstreamer_init(gint argc, gchar *argv[], const char* type);
void prepare_buffer(cv::Mat* input, int bw, int bh, int format);
void gstreamer_cleanup();
