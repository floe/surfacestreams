// based on:
//   * https://github.com/IntelRealSense/librealsense/tree/master/examples/align
//   * https://github.com/IntelRealSense/librealsense/tree/master/examples/measure
// tested with:
//   * librealsense2-2.10.4
//   * librealsense2-2.11.0

#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>
#include <iostream>
#include "common.h"

rs2::depth_frame* depthp;
rs2_intrinsics intrinsics;

void get_3d_point(int x, int y, float* out) {
    float px[2] = { (float)x, (float)y };
    rs2_deproject_pixel_to_point( out, &intrinsics, px, depthp->get_distance(x,y) );
}

void apply_to_color(rs2::depth_frame* depth, rs2_intrinsics* intr, float distance, PlaneModel<float>* plane, uint8_t* p_other_frame) {

  // blank out all remaining color pixels _below_ the plane
  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {

      float pt[3]; float px[2] = { x, y };
      rs2_deproject_pixel_to_point( pt, intr, px, depth->get_distance(x,y) );
      if (std::isnan(pt[2]) || std::isinf(pt[2]) || pt[2] <= 0) continue;
      Eigen::Vector3f point = { pt[0], pt[1], pt[2] };

      if ((plane->n.dot(point) - plane->d) > -distance*0.01) {
        int index = y*cw+x;
        ((uint32_t*)p_other_frame)[index] = 0x0000FF00;
      }
    }
  }
}

int main(int argc, char* argv[]) {

  bool _quit = false; quit = &_quit;
  if (argc > 2) gstpipe = argv[2];

  opencv_init(1280,720,1280,720);
  gstreamer_init(argc,argv,"RGBx");

  // Create a Pipeline - this serves as a top-level API for streaming and processing frames
  rs2::pipeline pipe;
  rs2::config cfg;
  rs2::colorizer color_map;

  cfg.enable_stream(RS2_STREAM_DEPTH, 1280, 720, RS2_FORMAT_Z16, 30);
  cfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_RGBA8, 30);

  // Configure and start the pipeline
  rs2::pipeline_profile profile = pipe.start( cfg );
  float depth_scale = profile.get_device().first<rs2::depth_sensor>().get_depth_scale();

  // TODO: At the moment the SDK does not offer a closed enum for D400 visual presets
  // (because they keep changing)
  // As a work-around we try to find the High-Density preset by name
  // We do this to reduce the number of black pixels
  // The hardware can perform hole-filling much better and much more power efficient then our software
  auto sensor = profile.get_device().first<rs2::depth_sensor>();
  auto range = sensor.get_option_range(RS2_OPTION_VISUAL_PRESET);
  for (auto i = range.min; i < range.max; i += range.step) {
		std::cout << sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i) << std::endl;
    if (std::string(sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i)) == "High Density")
      sensor.set_option(RS2_OPTION_VISUAL_PRESET, i);
	}

  sensor.set_option(RS2_OPTION_LASER_POWER, 360);
  sensor.set_option(RS2_OPTION_DEPTH_UNITS, 0.0001);

  // Create a rs2::align object.
  // rs2::align allows us to perform alignment of depth frames to others frames
  //The "align_to" is the stream type to which we plan to align depth frames.
  rs2::align align(RS2_STREAM_COLOR);

  auto stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
  intrinsics = stream.get_intrinsics(); // Calibration data

  while (!_quit) {

    // Block program until frames arrive
    rs2::frameset frames = pipe.wait_for_frames(); 
    
    //Get processed aligned frame
    auto processed = align.process(frames);

    rs2::video_frame color_frame = processed.first(RS2_STREAM_COLOR);

    // Try to get a frame of a depth image
    rs2::depth_frame depth = processed.get_depth_frame(); 
    depthp = &depth;

    //rs2::video_frame color_frame = color_map(depth);

    if (find_plane) {
      plane = ransac_plane(get_3d_point);
      find_plane = false;
    }

    // Get the depth frame's dimensions
    int width = depth.get_width();
    int height = depth.get_height();
    int other_bpp = color_frame.get_bytes_per_pixel();

		float clipping_dist = 1.0;

    const uint16_t* p_depth_frame = reinterpret_cast<const uint16_t*>(depth.get_data());
    uint8_t* p_other_frame = reinterpret_cast<uint8_t*>(const_cast<void*>(color_frame.get_data()));

		if (filter) apply_to_color(&depth,&intrinsics,distance,&plane,p_other_frame);

/*
    for (int y = 0; y < height; y++) {
        auto depth_pixel_index = y * width;
        for (int x = 0; x < width; x++, ++depth_pixel_index) {
            // Get the depth value of the current pixel
            auto pixels_distance = depth_scale * p_depth_frame[depth_pixel_index];

            // Check if the depth value is invalid (<=0) or greater than the threashold
            if (pixels_distance <= 0.f || pixels_distance > clipping_dist) {
                // Calculate the offset in other frame's buffer to current pixel
                auto offset = depth_pixel_index * other_bpp;
                // Set pixel to "background" color (0x999999)
                std::memset(&p_other_frame[offset], 0x99, other_bpp);
            }
        }
		}
*/
    // Query the distance from the camera to the object in the center of the image
    float dist_to_center = depth.get_distance(width / 2, height / 2);
    
    // Print the distance 
    std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";

    cv::Mat input(color_frame.get_height(),color_frame.get_width(),CV_8UC4,(uint8_t*)color_frame.get_data());
    prepare_buffer(&input,1280,720,CV_8UC4);
  }

	gstreamer_cleanup();
}
