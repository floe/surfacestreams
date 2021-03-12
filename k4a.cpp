// based on: https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/examples/viewer/opengl/main.cpp
// tested with: libk4a1.4-1.4.1

#include <iostream>
#include <k4a/k4a.hpp>

#include "common.h"

int main( int argc, char* argv [] ) {

	const uint32_t deviceCount = k4a::device::get_installed_count();
	if (deviceCount == 0) {
		throw std::runtime_error("No Azure Kinect devices detected!");
	}

  opencv_init(1280,720,512,512);
	gstreamer_init(argc,argv,"BGRA");

	// Start the device
	k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
	config.camera_fps = K4A_FRAMES_PER_SECOND_30;
	config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
	config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
	config.color_resolution = K4A_COLOR_RESOLUTION_720P;

	// This means that we'll only get captures that have both color and
	// depth images, so we don't need to check if the capture contains
	// a particular type of image.
	config.synchronized_images_only = true;

	std::cout << "Started opening K4A device..." << std::endl;

	k4a::device dev = k4a::device::open(K4A_DEVICE_DEFAULT);
	dev.start_cameras(&config);

	std::cout << "Finished opening K4A device." << std::endl;

	while (true) {

		k4a::capture capture;
		if (dev.get_capture(&capture, std::chrono::milliseconds(100))) {

			const k4a::image depthImage = capture.get_depth_image();
			const k4a::image colorImage = capture.get_color_image();

			// Depth data is in the form of uint16_t's representing the distance in
			// millimeters of the pixel from the camera,

			cv::Mat input(colorImage.get_height_pixels(),colorImage.get_width_pixels(),CV_8UC4,(uint8_t*)colorImage.get_buffer());
			prepare_buffer(&input,1280,720,CV_8UC4);

		}
	}
}
