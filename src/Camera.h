#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <vector>
#include <opencv2/core/core.hpp>
#include <PlaneModel.h>
#include <gst/gst.h>

// final transmitted width and height
extern const int tw, th;

class Camera {

	public:

		Camera(const char* _pipe, const char* _type, int _cw, int _ch, int _dw = 0, int _dh = 0, float _scale = 0.0f);

		virtual void retrieve_frames();
		virtual void remove_background();
		virtual void send_buffer();
		virtual void remove_background(float start, float end);

		virtual void push_point(float x, float y);
		virtual void handle_key(const char* key);

		void ransac_plane();

		bool do_quit;
		bool find_plane;
		bool do_filter;

	protected:

		int dw, dh, cw, ch;
		cv::Mat input;

		PlaneModel<float> plane;
		virtual void get_3d_pt(int x, int y, float* out);

		cv::Mat calcPerspective();
		void saveConfig();

		float distance; // plane segmentation distance in cm
		float scale; // scale from distance in cm to camera units
		cv::Mat im; // identity matrix
		cv::Mat pm; // perspective matrix

		void gstreamer_init(const char* type, const char* gstpipe);
		void gstreamer_cleanup();

		std::vector<cv::Point2f> src;
		std::vector<cv::Point2f> dst;

		GstElement* gpipeline;
		GstElement* appsrc;
		GstElement* conv;
		GstElement* videosink;
};

typedef struct {
	Camera* obj;
	float start,end;
} thread_info;

#endif // _CAMERA_H_