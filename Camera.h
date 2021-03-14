#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <vector>
#include <opencv2/core/core.hpp>
#include <PlaneModel.h>
#include <gst/gst.h>

class Camera {

	public:

		Camera(const char* _pipe, const char* _type, int _cw, int _ch, int _dw = 0, int _dh = 0);

		void process_frames(cv::Mat* color, cv::Mat* depth = nullptr);
		void prepare_buffer(cv::Mat* input, int format);

		void push_point(float x, float y);
		void handle_key(const char* key);
		bool do_quit;

	private:

		int dw, dh, cw, ch;

		PlaneModel<float> plane;
		void ransac_plane();
		void get_3d_pt(int x, int y, float* out);

		cv::Mat calcPerspective();

		float distance; // plane segmentation distance
		cv::Mat im; // identity matrix
		cv::Mat pm; // perspective matrix

		void gstreamer_init(const char* type, const char* gstpipe);
		void gstreamer_cleanup();

		bool find_plane;
		bool do_filter;

		std::vector<cv::Point2f> src;
		std::vector<cv::Point2f> dst;

		GstElement* gpipeline;
		GstElement* appsrc;
		GstElement* conv;
		GstElement* videosink;
};

#endif // _CAMERA_H_
