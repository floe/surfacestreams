#ifndef _VIRTUALCAM_H_
#define _VIRTUALCAM_H_

#include "Camera.h"

class VirtualCam: public Camera {

	public:

		VirtualCam(const char* pipe);

		void push_point(float x, float y);
		void handle_key(const char* key);

};

#endif // _VIRTUALCAM_H_
