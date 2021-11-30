// based on: https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/examples/viewer/opengl/main.cpp
// based on: https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/examples/transformation/main.cpp
// tested with: libk4a1.4-1.4.1

#include <iostream>

#include "KinectAzure.h"

KinectAzure::KinectAzure(const char* pipe): Camera(pipe,"BGRA",1280,720,1024,1024), scale(10) {

	const uint32_t deviceCount = k4a::device::get_installed_count();
	if (deviceCount == 0) 
		throw std::runtime_error("No Azure Kinect devices detected!");

	// Start the device
	k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
	config.camera_fps = K4A_FRAMES_PER_SECOND_15;
	config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED;
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_720P;
	config.synchronized_images_only = true;

	std::cout << "Started opening K4A device..." << std::endl;

	dev = k4a::device::open(K4A_DEVICE_DEFAULT);

	calibration = dev.get_calibration( config.depth_mode, config.color_resolution );
	transformation = k4a::transformation( calibration );

	dev.start_cameras(&config);

	std::cout << "Finished opening K4A device." << std::endl;
}

void KinectAzure::get_3d_pt(int x, int y, float* out) {
	k4a_float2_t pt; pt.xy.x = (float)x; pt.xy.y = (float)y;
	uint16_t* depthbuf = (uint16_t*)depthImage.get_buffer();
	float depth = depthbuf[y*dw+x];
	bool res = calibration.convert_2d_to_3d( pt, depth, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, (k4a_float3_t*)out );
	if (!res) out[2] = -1.0f;
}

void KinectAzure::retrieve_frames() {
	k4a::capture capture;
	if (dev.get_capture(&capture, std::chrono::milliseconds(100))) {
		depthImage = capture.get_depth_image();
		colorImage = capture.get_color_image();
	}
}

typedef struct {
	KinectAzure* obj;
	int start,end;
} thread_info;

void* thread_helper(void* arg) {
	thread_info* ti = (thread_info*)arg;
	ti->obj->blank_depth(ti->start,ti->end);
	return nullptr;
}

void KinectAzure::remove_background() {

	if (depthImage.handle() == nullptr) return;

	thread_info ti = { this, 0, dh/2 };
	pthread_t thread;
	pthread_create(&thread,nullptr,&thread_helper,(void*)&ti);
	blank_depth(dh/2+1,dh);
	pthread_join(thread,nullptr);
	map_to_color();

}

void KinectAzure::blank_depth(int ystart, int yend) {

			// FIXME: this function eats up an inordinate amount of processing time, parallelize?
			int index = ystart*dw;
			uint16_t dv = 0;
			float out[3];
			k4a_float2_t pt;

			// in the original depth image, zero all pixels with invalid data or below the plane
			uint16_t* depthData = (uint16_t*)depthImage.get_buffer();
			for (int y = ystart; y < yend; ++y) {
				for (int x = 0; x < dw; ++x) {

					dv = depthData[index];
					if (dv == 0) { index += 1; continue; }

					pt.xy.x = (float)x;
					pt.xy.y = (float)y;

					bool res = calibration.convert_2d_to_3d( pt, (float)dv, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_COLOR, (k4a_float3_t*)out );
					if (!res) { index += 1; continue; }

					Eigen::Vector3f point = { out[0], out[1], out[2] };
					if ((plane.n.dot(point) - plane.d) > -distance*scale)
						depthData[index] = 0;

					index += 1;
				}
			}
}

void KinectAzure::map_to_color() {

			k4a::image transformed_depth_image = transformation.depth_image_to_color_camera(depthImage);

			const int width = transformed_depth_image.get_width_pixels();
			const int height = transformed_depth_image.get_height_pixels();

			assert( width == cw);
			assert(height == ch);

			uint32_t* buffer = (uint32_t*)colorImage.get_buffer();
			uint16_t* depthData = (uint16_t*)transformed_depth_image.get_buffer();
			if (do_filter) {
				for (int index = 0; index < cw*ch; index++) {
					uint16_t dv = depthData[index];
					if (dv == 0) 
						buffer[index] = 0x0000FF00;
				}
			}

			input = cv::Mat(colorImage.get_height_pixels(),colorImage.get_width_pixels(),CV_8UC4,(uint8_t*)colorImage.get_buffer());
}
