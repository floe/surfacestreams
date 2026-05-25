#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <opencv2/core/core.hpp>
#include <gst/gst.h>
#include <TuioOverlay.h>
#include <Calibrator.h>

class Camera {

	public:

		Camera(const char* _pipe, const char* _type, int _cw, int _ch, int _tw = 1280, int _th = 720 );

		virtual void retrieve_frames() = 0;
		virtual void remove_background();
		virtual void send_buffer( bool do_blank = false );
		virtual void remove_background(float start, float end);

		void setPadHandler( GstPadEventFunction pad_event );

		virtual void ransac_plane();
		bool auto_perspective();
		void undistort();

		Calibrator calib;

		virtual cv::FileStorage saveConfig();
		virtual cv::FileStorage loadConfig();

	protected:

		int cw, ch; // input color image size
		int tw, th; // final transmitted image size
		cv::Mat input; // input camera image

		// undistortion
		cv::Mat camMat, distCoeffs;
		cv::Mat map1, map2;

		void gstreamer_init(const char* type, const char* gstpipe);
		void gstreamer_cleanup();

		GstElement* gpipeline;
		GstElement* appsrc;
		GstElement* conv;
		GstElement* videosink;

		TuioOverlay overlay;
};

#endif // _CAMERA_H_
