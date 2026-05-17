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

		virtual void push_point(float x, float y);
		virtual void handle_key(const char* key);

		void ransac_plane();
		void autoPerspective();

		bool find_plane;

	protected:

		PlaneModel<float> plane;
		virtual void get_3d_pt(int x, int y, float* out);

};

#endif // _DEPTHCAMERA_H_
