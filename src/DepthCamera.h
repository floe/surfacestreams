#ifndef _DEPTHCAMERA_H_
#define _DEPTHCAMERA_H_

#include "Camera.h"
#include <vector>
#include <opencv2/core/core.hpp>
#include <PlaneModel.h>
#include <TUIO/TuioClient.h>

class DepthCamera: public Camera {

	public:

		DepthCamera(const char* _pipe, const char* _type, int _cw, int _ch, int _dw = 0, int _dh = 0, float _scale = 0.0f, int _tw = 1280, int _th = 720 );

		void ransac_plane();

		bool find_plane;

	protected:

		int dw,dh; // input depth image size

		float distance; // plane segmentation distance in cm
		float scale; // scale from distance in cm to camera units

		virtual cv::FileStorage saveConfig();
		virtual cv::FileStorage loadConfig();

		PlaneModel<float> plane;
		virtual void get_3d_pt(int x, int y, float* out);

};

#endif // _DEPTHCAMERA_H_
