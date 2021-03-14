#ifndef _SUR40_H_
#define _SUR40_H_

#include "Camera.h"

class SUR40: Camera {

	SUR40(const char* pipe);

	void process_frames(cv::Mat* color, cv::Mat* depth = nullptr);

};

#endif // _SUR40_H_
