#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <vector>
#include <opencv2/core/core.hpp>
#include <gst/gst.h>
#include <TuioOverlay.h>
#include <Calibrator.h>

extern bool do_filter;
extern bool do_blank;

class Camera {

	public:

		Camera(const char* _pipe, const char* _type, int _cw, int _ch, int _tw = 1280, int _th = 720 );

		virtual void retrieve_frames();
		virtual void remove_background();
		virtual void send_buffer();
		virtual void remove_background(float start, float end);

		virtual void handle_key(const char* key);

		virtual void ransac_plane();
		void autoPerspective();

		Calibrator calib;

		bool do_quit;
		bool find_plane;
		bool autocalib;

	protected:

		int cw, ch; // input color image size
		int tw, th; // final transmitted image size
		cv::Mat input; // input camera image

		// undistortion
		cv::Mat camMat, distCoeffs;
		cv::Mat map1, map2;

		virtual cv::FileStorage saveConfig();
		virtual cv::FileStorage loadConfig();

		void gstreamer_init(const char* type, const char* gstpipe);
		void gstreamer_cleanup();

		GstElement* gpipeline;
		GstElement* appsrc;
		GstElement* conv;
		GstElement* videosink;

		TuioOverlay overlay;
};

#endif // _CAMERA_H_
