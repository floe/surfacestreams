/*
 * This file is part of the OpenKinect Project. http://www.openkinect.org
 *
 * Copyright (c) 2011 individual OpenKinect contributors. See the CONTRIB file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

/** @file Protonect.cpp Main application file. */

#include <iostream>
#include <cstdlib>
#include <signal.h>

/// [headers]
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>
/// [headers]
#ifdef EXAMPLES_WITH_OPENGL_SUPPORT
#include "viewer.h"
#endif

////////////////////////////////////////////////////////////////////////////////
//
// OpenCV stuff
//

#include <Eigen/Core>
#include <SimpleRansac.h>
#include <PlaneModel.h>

PlaneModel<float> plane;

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

using namespace cv;

std::vector<Point2f> src;
std::vector<Point2f> dst;

// im: identity matrix
Mat im = (Mat_<float>(3,3) << 1280.0/1920.0, 0, 0, 0, 720.0/1080.0, 0, 0, 0, 1 );
// pm: perspective matrix
Mat pm = im;

Mat calcPerspective() {

  Mat result;

  dst.push_back(Point2f(   0,  0));
  dst.push_back(Point2f(1280,  0));
  dst.push_back(Point2f(1280,720));
  dst.push_back(Point2f(   0,720));

  result = getPerspectiveTransform(src,dst);

  src.clear();
  dst.clear();

  return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// GStreamer stuff
//

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/navigation.h>

#include <stdint.h>
#include <string.h>

#include <immintrin.h>

bool find_plane = true;
bool filter = true;
bool quit = false;

float distance = 5;

GstElement *gpipeline, *appsrc, *conv, *videosink;

gboolean pad_event(GstPad *pad, GstObject *parent, GstEvent *event) {

  if (GST_EVENT_TYPE (event) != GST_EVENT_NAVIGATION)
    return gst_pad_event_default(pad,parent,event);

  double x,y; int b;
  const gchar* key;

  switch (gst_navigation_event_get_type(event)) {

    // calibration: top (left, right), bottom (left, right)
    case GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE:
      gst_navigation_event_parse_mouse_button_event(event,&b,&x,&y);
      src.push_back(Point2f(1920.0*x/1280.0,1080.0*y/720.0));
      break;

    case GST_NAVIGATION_EVENT_KEY_PRESS:
      gst_navigation_event_parse_key_event(event,&key);

      // reset perspective matrix
      if (key == std::string("space"))
        pm = im;

      // find largest plane
      if (key == std::string("p"))
        find_plane = true;

      // subtract plane
      if (key == std::string("f"))
        filter = !filter;

      // quit
      if (key == std::string("q"))
        quit = true;

      // change plane distance threshold
      if (key == std::string( "plus")) distance += 0.5;
      if (key == std::string("minus")) distance -= 0.5;
      break;

    default:
      return false;
  }

  if (src.size() == 4) {
    Mat r = calcPerspective();
    pm = r; //set_matrix(perspective,r.ptr<gdouble>(0));
  }

  return true;
}

void buffer_destroy(gpointer data) {
  libfreenect2::Frame* done = (libfreenect2::Frame*)data;
  delete done;
}

GstFlowReturn prepare_buffer(GstAppSrc* appsrc, libfreenect2::Frame* frame) {

  guint size = 1280 * 720 * 4;
  GstBuffer *buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, (gpointer)(frame->data), size, 0, size, frame, buffer_destroy );

  return gst_app_src_push_buffer(appsrc, buffer);
}

void gstreamer_init(gint argc, gchar *argv[]) {

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* setup pipeline */
  gpipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");

  // attach event listener to appsrc pad
  GstPad* srcpad = gst_element_get_static_pad (appsrc, "src");
  gst_pad_set_event_function( srcpad, (GstPadEventFunction)pad_event );

  // create pipeline from string
  const char* pipe_desc = /*argv[2] ? argv[2] : */ "videoconvert ! fpsdisplaysink sync=false";
  videosink = gst_parse_bin_from_description(pipe_desc,TRUE,NULL);

  /* setup */
  g_object_set (G_OBJECT (appsrc), "caps",
    gst_caps_new_simple ("video/x-raw",
				     "format", G_TYPE_STRING, "BGRA",
				     "width", G_TYPE_INT, 1280,
				     "height", G_TYPE_INT, 720,
				     "framerate", GST_TYPE_FRACTION, 0, 1,
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


// Protonect stuff

bool protonect_shutdown = false; ///< Whether the running application should shut down.

void sigint_handler(int s)
{
  protonect_shutdown = true;
}

bool protonect_paused = false;
libfreenect2::Freenect2Device *devtopause;

//Doing non-trivial things in signal handler is bad. If you want to pause,
//do it in another thread.
//Though libusb operations are generally thread safe, I cannot guarantee
//everything above is thread safe when calling start()/stop() while
//waitForNewFrame().
void sigusr1_handler(int s)
{
  if (devtopause == 0)
    return;
/// [pause]
  if (protonect_paused)
    devtopause->start();
  else
    devtopause->stop();
  protonect_paused = !protonect_paused;
/// [pause]
}

//The following demostrates how to create a custom logger
/// [logger]
#include <fstream>
#include <cstdlib>
class MyFileLogger: public libfreenect2::Logger
{
private:
  std::ofstream logfile_;
public:
  MyFileLogger(const char *filename)
  {
    if (filename)
      logfile_.open(filename);
    level_ = Debug;
  }
  bool good()
  {
    return logfile_.is_open() && logfile_.good();
  }
  virtual void log(Level level, const std::string &message)
  {
    logfile_ << "[" << libfreenect2::Logger::level2str(level) << "] " << message << std::endl;
  }
};
/// [logger]

/// [main]
/**
 * Main application entry point.
 *
 * Accepted argumemnts:
 * - cpu Perform depth processing with the CPU.
 * - gl  Perform depth processing with OpenGL.
 * - cl  Perform depth processing with OpenCL.
 * - <number> Serial number of the device to open.
 * - -noviewer Disable viewer window.
 */
int main(int argc, char *argv[])
/// [main]
{
  std::string program_path(argv[0]);
  std::cerr << "Version: " << LIBFREENECT2_VERSION << std::endl;
  std::cerr << "Environment variables: LOGFILE=<protonect.log>" << std::endl;
  std::cerr << "Usage: " << program_path << " [-gpu=<id>] [gl | cl | cuda | cpu] [<device serial>]" << std::endl;
  std::cerr << "        [-noviewer] [-norgb | -nodepth] [-help] [-version]" << std::endl;
  std::cerr << "        [-frames <number of frames to process>]" << std::endl;
  std::cerr << "To pause and unpause: pkill -USR1 Protonect" << std::endl;
  size_t executable_name_idx = program_path.rfind("Protonect");

  std::string binpath = "/";

  if(executable_name_idx != std::string::npos)
  {
    binpath = program_path.substr(0, executable_name_idx);
  }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  // avoid flooing the very slow Windows console with debug messages
  libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Info));
#else
  // create a console logger with debug level (default is console logger with info level)
/// [logging]
  libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
/// [logging]
#endif
/// [file logging]
  MyFileLogger *filelogger = new MyFileLogger(getenv("LOGFILE"));
  if (filelogger->good())
    libfreenect2::setGlobalLogger(filelogger);
  else
    delete filelogger;
/// [file logging]

/// [context]
  libfreenect2::Freenect2 freenect2;
  libfreenect2::Freenect2Device *dev = 0;
  libfreenect2::PacketPipeline *pipeline = 0;
/// [context]

  std::string serial = "";

  bool viewer_enabled = true;
  bool enable_rgb = true;
  bool enable_depth = true;
  int deviceId = -1;
  size_t framemax = -1;

  for(int argI = 1; argI < argc; ++argI)
  {
    const std::string arg(argv[argI]);

    if(arg == "-help" || arg == "--help" || arg == "-h" || arg == "-v" || arg == "--version" || arg == "-version")
    {
      // Just let the initial lines display at the beginning of main
      return 0;
    }
    else if(arg.find("-gpu=") == 0)
    {
      if (pipeline)
      {
        std::cerr << "-gpu must be specified before pipeline argument" << std::endl;
        return -1;
      }
      deviceId = atoi(argv[argI] + 5);
    }
    else if(arg == "cpu")
    {
      if(!pipeline)
/// [pipeline]
        pipeline = new libfreenect2::CpuPacketPipeline();
/// [pipeline]
    }
    else if(arg == "gl")
    {
#ifdef LIBFREENECT2_WITH_OPENGL_SUPPORT
      if(!pipeline)
        pipeline = new libfreenect2::OpenGLPacketPipeline();
#else
      std::cout << "OpenGL pipeline is not supported!" << std::endl;
#endif
    }
    else if(arg == "cl")
    {
#ifdef LIBFREENECT2_WITH_OPENCL_SUPPORT
      if(!pipeline)
        pipeline = new libfreenect2::OpenCLPacketPipeline(deviceId);
#else
      std::cout << "OpenCL pipeline is not supported!" << std::endl;
#endif
    }
    else if(arg == "cuda")
    {
#ifdef LIBFREENECT2_WITH_CUDA_SUPPORT
      if(!pipeline)
        pipeline = new libfreenect2::CudaPacketPipeline(deviceId);
#else
      std::cout << "CUDA pipeline is not supported!" << std::endl;
#endif
    }
    else if(arg.find_first_not_of("0123456789") == std::string::npos) //check if parameter could be a serial number
    {
      serial = arg;
    }
    else if(arg == "-noviewer" || arg == "--noviewer")
    {
      viewer_enabled = false;
    }
    else if(arg == "-norgb" || arg == "--norgb")
    {
      enable_rgb = false;
    }
    else if(arg == "-nodepth" || arg == "--nodepth")
    {
      enable_depth = false;
    }
    else if(arg == "-frames")
    {
      ++argI;
      framemax = strtol(argv[argI], NULL, 0);
      if (framemax == 0) {
        std::cerr << "invalid frame count '" << argv[argI] << "'" << std::endl;
        return -1;
      }
    }
    else
    {
      std::cout << "Unknown argument: " << arg << std::endl;
    }
  }

  if (!enable_rgb && !enable_depth)
  {
    std::cerr << "Disabling both streams is not allowed!" << std::endl;
    return -1;
  }

/// [discovery]
  if(freenect2.enumerateDevices() == 0)
  {
    std::cout << "no device connected!" << std::endl;
    return -1;
  }

  if (serial == "")
  {
    serial = freenect2.getDefaultDeviceSerialNumber();
  }
/// [discovery]

  if(pipeline)
  {
/// [open]
    dev = freenect2.openDevice(serial, pipeline);
/// [open]
  }
  else
  {
    dev = freenect2.openDevice(serial);
  }

  if(dev == 0)
  {
    std::cout << "failure opening device!" << std::endl;
    return -1;
  }

  devtopause = dev;

  signal(SIGINT,sigint_handler);
#ifdef SIGUSR1
  signal(SIGUSR1, sigusr1_handler);
#endif
  protonect_shutdown = false;

/// [listeners]
  int types = 0;
  if (enable_rgb)
    types |= libfreenect2::Frame::Color;
  if (enable_depth)
    types |= libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
  libfreenect2::SyncMultiFrameListener listener(types);
  libfreenect2::FrameMap frames;

  dev->setColorFrameListener(&listener);
  dev->setIrAndDepthFrameListener(&listener);
/// [listeners]

  gstreamer_init(argc,argv);

/// [start]
  if (enable_rgb && enable_depth)
  {
    if (!dev->start())
      return -1;
  }
  else
  {
    if (!dev->startStreams(enable_rgb, enable_depth))
      return -1;
  }

  std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
  std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;
/// [start]

/// [registration setup]
  libfreenect2::Registration* registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
  libfreenect2::Frame undistorted(512, 424, 4), registered(512, 424, 4);
  int color_map[512*424];
/// [registration setup]

  size_t framecount = 0;
#ifdef EXAMPLES_WITH_OPENGL_SUPPORT
  Viewer viewer;
  if (viewer_enabled)
    viewer.initialize();
#else
  viewer_enabled = false;
#endif

/// [loop start]
  while(!protonect_shutdown && (framemax == (size_t)-1 || framecount < framemax))
  {
    if (!listener.waitForNewFrame(frames, 10*1000)) // 10 sconds
    {
      std::cout << "timeout!" << std::endl;
      return -1;
    }
    libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
    libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
    libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];
/// [loop start]

    if (enable_rgb && enable_depth)
    {
/// [registration]
      registration->apply(rgb, depth, &undistorted, &registered, false, NULL, color_map);
/// [registration]
    }

/// [gstreamer]

    // use RANSAC to compute a plane out of sparse point cloud
    std::vector<Eigen::Vector3f> points;
    int dw = 512, dh = 424;
    int cw = 1920, ch = 1080;
    if (find_plane) {
      for (int y = 0; y < dh; y++) {
        for (int x = 0; x < dw; x++) {
          float px,py,pz;
          registration->getPointXYZ(&undistorted,y,x,px,py,pz);
          if (std::isnan(pz) || std::isinf(pz) || pz <= 0) continue;
          Eigen::Vector3f point = { px, py, pz };
          points.push_back( point );
        }
      }

      std::cout << "3D point count: " << points.size() << std::endl;
      plane = ransac<PlaneModel<float>>( points, distance*0.01, 200 );
      std::cout << "Ransac computed plane: n=" << plane.n.transpose() << " d=" << plane.d << std::endl;
      find_plane = false;
    }

    for (int y = 0; y < dh; y++) {
      for (int x = 0; x < dw; x++) {

        float px,py,pz;
        registration->getPointXYZ(&undistorted,y,x,px,py,pz);
        if (std::isnan(pz) || std::isinf(pz) || pz <= 0) continue;
        Eigen::Vector3f point = { px, py, pz };

        if (filter) if (fabs(plane.n.dot(point) - plane.d) < distance*0.01) {
          int index = color_map[y*dw+x];

          // creates rectangular patch of size 5x3
          if (index < cw+2 || index >= cw*(ch-1)-2) continue;
          for (int ty = -cw; ty <= cw; ty += cw)
            for (int tx = -2; tx <= 2; tx++)
              ((uint32_t*)rgb->data)[index+ty+tx] = 0;
        }
      }
    }

    libfreenect2::Frame *newrgb = new libfreenect2::Frame(1280, 720, 4);

    Mat input(1080,1920,CV_8UC4,rgb->data);
    Mat output(720,1280,CV_8UC4,newrgb->data);
    warpPerspective(input,output,pm,output.size(),INTER_NEAREST);

    prepare_buffer((GstAppSrc*)appsrc,newrgb);
    g_main_context_iteration(g_main_context_default(),FALSE);

/// [gstreamer]

    framecount++;
    if (!viewer_enabled)
    {
      if (framecount % 100 == 0)
        std::cout << "The viewer is turned off. Received " << framecount << " frames. Ctrl-C to stop." << std::endl;
      listener.release(frames);
      continue;
    }

#ifdef EXAMPLES_WITH_OPENGL_SUPPORT
    if (enable_rgb)
    {
      viewer.addFrame("RGB", rgb);
    }
    if (enable_depth)
    {
      viewer.addFrame("ir", ir);
      viewer.addFrame("depth", depth);
    }
    if (enable_rgb && enable_depth)
    {
      viewer.addFrame("registered", &registered);
    }

    protonect_shutdown = protonect_shutdown || viewer.render();
#endif

/// [loop end]
    listener.release(frames);
    /** libfreenect2::this_thread::sleep_for(libfreenect2::chrono::milliseconds(100)); */
  }
/// [loop end]

  // TODO: restarting ir stream doesn't work!
  // TODO: bad things will happen, if frame listeners are freed before dev->stop() :(
/// [stop]
  dev->stop();
  dev->close();
/// [stop]

  delete registration;

  /* clean up */
  gst_element_set_state (gpipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (gpipeline));

  return 0;
}
