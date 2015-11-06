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
#include <signal.h>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>
#ifdef EXAMPLES_WITH_OPENGL_SUPPORT
#include "viewer.h"
#endif


// GStreamer stuff

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <stdint.h>

#include <immintrin.h>

GstElement *gpipeline, *appsrc, *conv, *videosink;

void buffer_destroy(gpointer data) {
  libfreenect2::Frame* done = (libfreenect2::Frame*)data;
  delete done;
}

GstFlowReturn prepare_buffer(GstAppSrc* appsrc, libfreenect2::Frame* frame) {

  guint size = 1920 * 1080 * 4;
  GstBuffer *buffer = gst_buffer_new_wrapped_full( (GstMemoryFlags)0, (gpointer)(frame->data), size, 0, size, frame, buffer_destroy );

  return gst_app_src_push_buffer(appsrc, buffer);
}


void gstreamer_init(gint argc, gchar *argv[]) {

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* setup pipeline */
  gpipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");

  const char* pipe_desc = argv[2] ? argv[2] : "videoconvert ! autovideosink";
  videosink = gst_parse_bin_from_description(pipe_desc,TRUE,NULL);

  /* setup */
  g_object_set (G_OBJECT (appsrc), "caps",
    gst_caps_new_simple ("video/x-raw",
				     "format", G_TYPE_STRING, "BGRA",
				     "width", G_TYPE_INT, 1920,
				     "height", G_TYPE_INT, 1080,
				     "framerate", GST_TYPE_FRACTION, 0, 1,
				     NULL), NULL);
  gst_bin_add_many (GST_BIN (gpipeline), appsrc, videosink, NULL);
  gst_element_link_many (appsrc, videosink, NULL);

  /* setup appsrc */
  g_object_set (G_OBJECT (appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
    "is-live", TRUE,
    "min-latency", 0,
    "max-latency", gst_util_uint64_scale_int (1, GST_SECOND, 30),
    "do-timestamp", TRUE,
    NULL);

  /* play */
  gst_element_set_state (gpipeline, GST_STATE_PLAYING);
}


// Protonect stuff

bool protonect_shutdown = false; ///< Whether the running application should shut down.

libfreenect2::Freenect2 freenect2;
libfreenect2::Freenect2Device *dev = 0;
libfreenect2::PacketPipeline *pipeline = 0;

libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth);
libfreenect2::FrameMap frames;
libfreenect2::Frame undistorted(512, 424, 4), registered(512, 424, 4), bigdepth(1920,1080+2,4);
libfreenect2::Registration* registration = 0;

void sigint_handler(int s)
{
  protonect_shutdown = true;
}

//The following demostrates how to create a custom logger
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
int setup(int argc, char* argv[])
{
  std::string program_path(argv[0]);
  std::cerr << "Environment variables: LOGFILE=<protonect.log>" << std::endl;
  std::cerr << "Usage: " << program_path << " [gl | cl | cpu] [<device serial>] [-noviewer]" << std::endl;
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
  libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
#endif
  MyFileLogger *filelogger = new MyFileLogger(getenv("LOGFILE"));
  if (filelogger->good())
    libfreenect2::setGlobalLogger(filelogger);

  if(freenect2.enumerateDevices() == 0)
  {
    std::cout << "no device connected!" << std::endl;
    return -1;
  }

  std::string serial = freenect2.getDefaultDeviceSerialNumber();

  bool viewer_enabled = true;

  for(int argI = 1; argI < argc; ++argI)
  {
    const std::string arg(argv[argI]);

    if(arg == "cpu")
    {
      if(!pipeline)
        pipeline = new libfreenect2::CpuPacketPipeline();
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
        pipeline = new libfreenect2::OpenCLPacketPipeline();
#else
      std::cout << "OpenCL pipeline is not supported!" << std::endl;
#endif
    }
    else if(arg.find_first_not_of("0123456789") == std::string::npos) //check if parameter could be a serial number
    {
      serial = arg;
    }
    else if(arg == "-noviewer")
    {
      viewer_enabled = false;
    }
    else
    {
      std::cout << "Unknown argument: " << arg << std::endl;
    }
  }

  if(pipeline)
  {
    dev = freenect2.openDevice(serial, pipeline);
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

  signal(SIGINT,sigint_handler);
  protonect_shutdown = false;

  dev->setColorFrameListener(&listener);
  dev->setIrAndDepthFrameListener(&listener);
  dev->start();

  std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
  std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;

  registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
}

void handle_frame() {
  listener.waitForNewFrame(frames);
  libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
  libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
  libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];

  // clip depth to max. 1m
  float* depth_s = ((float*)depth->data);
  float limit = 1000.0;
  __m256 ymm0, ymm1, ymm2, ymm3 = _mm256_broadcast_ss(&limit);
  for (int i = 0; i < (512*424)/8; i++, depth_s+=8) {
    ymm0 = _mm256_load_ps(depth_s);
    ymm1 = _mm256_cmp_ps(ymm0,ymm3,_CMP_LE_OS);
    ymm2 = _mm256_and_ps(ymm0,ymm1);
    _mm256_store_ps(depth_s,ymm2);
    //if (*depth_s > 1000.0) *depth_s = 0;
  }

  registration->apply(rgb,depth,&undistorted,&registered,true,&bigdepth);

  float* depth_p = ((float*)bigdepth.data)+1920;
  unsigned int* rgb_src = (unsigned int*)rgb->data;
  __m256 ymm4, ymm5 = _mm256_broadcast_ss(&limit);
  for (int i = 0; i < (1920*1080)/8; i++, depth_p+=8, rgb_src+=8) {
    ymm0 = _mm256_load_ps(depth_p);
    ymm1 = _mm256_cmp_ps(ymm0,ymm5,_CMP_LE_OS);
    ymm4 = (__m256)_mm256_load_si256((__m256i*)rgb_src);
    ymm2 = _mm256_and_ps(ymm4,ymm1);
    _mm256_store_si256((__m256i*)rgb_src,(__m256i)ymm2);
  }

  prepare_buffer((GstAppSrc*)appsrc,rgb);
  g_main_context_iteration(g_main_context_default(),FALSE);

  // don't let the listener free the frames, it will steal the rgb frame from the appsrc otherwise
  delete depth;
  delete ir;
}

int main(int argc, char *argv[])
{
  setup(argc,argv);
  gstreamer_init(argc,argv);

  while(!protonect_shutdown)
  {
    handle_frame();
  }

  // TODO: restarting ir stream doesn't work!
  // TODO: bad things will happen, if frame listeners are freed before dev->stop() :(
  dev->stop();
  dev->close();

  delete registration;

  /* clean up */
  gst_element_set_state (gpipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (gpipeline));

  return 0;
}
