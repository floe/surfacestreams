#ifndef _V4L2_H_
#define _V4L2_H_

#include "Camera.h"

#include <opencv2/videoio.hpp>

class V4L2: public Camera {

	public:

		V4L2(const char* pipe, int devnum, int cw, int ch);

		void retrieve_frames();

	protected:

		cv::VideoCapture cap;

};

#endif // _V4L2_H_
