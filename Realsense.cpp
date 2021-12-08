// based on:
//   * https://github.com/IntelRealSense/librealsense/tree/master/examples/align
//   * https://github.com/IntelRealSense/librealsense/tree/master/examples/measure
// tested with:
//   * librealsense2-2.49.0

#include "Realsense.h"
#include <iostream>

Realsense::Realsense(const char* pipe):
  Camera(pipe,"RGBx",1280,720,1280,720,0.01f),
  // align parameter is the stream type to which we plan to align depth frames.
  align(RS2_STREAM_COLOR),
  depth_frame(nullptr),
  color_frame(nullptr)
{
  // Create a Pipeline - this serves as a top-level API for streaming and processing frames
  rs2::config cfg;

  cfg.enable_stream(RS2_STREAM_DEPTH, dw, dh, RS2_FORMAT_Z16, 30);
  cfg.enable_stream(RS2_STREAM_COLOR, cw, ch, RS2_FORMAT_RGBA8, 30);

  // Configure and start the pipeline
  rs2::pipeline_profile profile = pipeline.start( cfg );

  // TODO: At the moment the SDK does not offer a closed enum for D400 visual presets
  // (because they keep changing)
  // As a work-around we try to find the High-Density preset by name
  // We do this to reduce the number of black pixels
  // The hardware can perform hole-filling much better and much more power efficient then our software
  auto sensor = profile.get_device().first<rs2::depth_sensor>();
  auto range = sensor.get_option_range(RS2_OPTION_VISUAL_PRESET);
  for (auto i = range.min; i < range.max; i += range.step) {
    //std::cout << sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i) << std::endl;
    if (std::string(sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i)) == "High Density")
      sensor.set_option(RS2_OPTION_VISUAL_PRESET, i);
  }

  // FIXME: set to max
  sensor.set_option(RS2_OPTION_LASER_POWER, 360);
  sensor.set_option(RS2_OPTION_DEPTH_UNITS, 0.0001);

  auto stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
  intrinsics = stream.get_intrinsics(); // Calibration data
}

void Realsense::get_3d_pt(int x, int y, float* out) {
    float px[2] = { (float)x, (float)y };
    rs2_deproject_pixel_to_point( out, &intrinsics, px, depth_frame.get_distance(x,y) );
}

void Realsense::apply_to_color(rs2::depth_frame& depth, rs2_intrinsics* intr, float distance, PlaneModel<float>* plane, uint8_t* p_other_frame) {

  // blank out all remaining color pixels _below_ the plane
  for (int y = 0; y < ch; y++) {
    for (int x = 0; x < cw; x++) {

      float pt[3]; float px[2] = { (float)x, (float)y };
      rs2_deproject_pixel_to_point( pt, intr, px, depth_frame.get_distance(x,y) );
      if (std::isnan(pt[2]) || std::isinf(pt[2]) || pt[2] <= 0) continue;
      Eigen::Vector3f point = { pt[0], pt[1], pt[2] };

      if ((plane->n.dot(point) - plane->d) > -distance*scale) {
        int index = y*cw+x;
        ((uint32_t*)p_other_frame)[index] = 0x0000FF00;
      }
    }
  }
}

void Realsense::retrieve_frames() {

    // Block program until frames arrive
    rs2::frameset frames = pipeline.wait_for_frames(); 
    
    //Get processed aligned frame
    auto processed = align.process(frames);

    color_frame = processed.first(RS2_STREAM_COLOR);

    // Try to get a frame of a depth image
    depth_frame = processed.get_depth_frame(); 
}

void Realsense::remove_background() {

    // Get the depth frame's dimensions
    int width = depth_frame.get_width();
    int height = depth_frame.get_height();

    uint8_t* p_other_frame = reinterpret_cast<uint8_t*>(const_cast<void*>(color_frame.get_data()));

    if (do_filter) apply_to_color(depth_frame,&intrinsics,distance,&plane,p_other_frame);

    // Query the distance from the camera to the object in the center of the image
    float dist_to_center = depth_frame.get_distance(width / 2, height / 2);
    
    // Print the distance 
    std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";

    input = cv::Mat(color_frame.get_height(),color_frame.get_width(),CV_8UC4,(uint8_t*)color_frame.get_data());
}

