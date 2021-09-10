#ifndef _REALSENSE_H_
#define _REALSENSE_H_

#include "Camera.h"
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

class Realsense: public Camera {

	public:

		Realsense(const char* pipe);

		void retrieve_frames();
		void remove_background();
		void get_3d_pt(int x, int y, float* out);
		void blank_depth(int ystart, int yend);

	private:

		void apply_to_color(rs2::depth_frame& depth, rs2_intrinsics* intr, float distance, PlaneModel<float>* plane, uint8_t* p_other_frame);

		rs2::align align;

		rs2::depth_frame depth_frame;
		rs2::video_frame color_frame;

		rs2_intrinsics intrinsics;
    rs2::pipeline pipeline;
};

#endif // _REALSENSE_H_
