#ifndef _KINECTAZURE_H_
#define _KINECTAZURE_H_

#include "Camera.h"
#include <k4a/k4a.hpp>

class KinectAzure: public Camera {

	public:

		KinectAzure(const char* pipe);

		void retrieve_frames();
		void remove_background();
		void get_3d_pt(int x, int y, float* out);
		void blank_depth(int ystart, int yend);

	private:

		void map_to_color();

		k4a::device dev;

		k4a::calibration calibration;
		k4a::transformation transformation;

		k4a::image depthImage;
		k4a::image colorImage;
};

#endif // _KINECTAZURE_H_
