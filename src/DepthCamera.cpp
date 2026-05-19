#include "DepthCamera.h"

////////////////////////////////////////////////////////////////////////////////
//
// plane model stuff
//

#include <Eigen/Core>
#include <SimpleRansac.h>
#include <PlaneModel.h>
#include <opencv2/core/eigen.hpp>

DepthCamera::DepthCamera(const char* _pipe, const char* _type, int _cw, int _ch, int _dw, int _dh, float _scale, int _tw, int _th ):
  Camera(_pipe,_type,_cw,_ch,_tw,_th) {
    dw = _dw; dh = _dh;
    find_plane = false;
    distance = 1.0f;
    scale = _scale;
}

cv::FileStorage DepthCamera::saveConfig() {

  cv::Mat tmp; cv::eigen2cv(plane.n,tmp);
  cv::FileStorage file = Camera::saveConfig();

  file << "distance" << distance;
  file << "plane_d" << plane.d;
  file << "plane_n" << tmp;

  return file;
}

cv::FileStorage DepthCamera::loadConfig() {

  cv::Mat tmp;
  cv::FileStorage file = Camera::loadConfig();

  file["distance"] >> distance;
  file["plane_d"] >> plane.d;
  file["plane_n"] >> tmp;
  cv::cv2eigen(tmp,plane.n);

  return file;
}

// use RANSAC to compute a plane out of sparse point cloud
void DepthCamera::ransac_plane() {

  std::vector<Eigen::Vector3f> points;

  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {
      float pt[3];
      get_3d_pt(x,y,pt);
      if (std::isnan(pt[2]) || std::isinf(pt[2]) || pt[2] <= 0) continue;
      Eigen::Vector3f point = { pt[0], pt[1], pt[2] };
      points.push_back( point );
    }
  }

  std::cout << "3D point count: " << points.size() << std::endl;
  plane = ransac<PlaneModel<float>>( points, distance*scale, 200 );
  if (plane.d < 0.0) { plane.d = -plane.d; plane.n = -plane.n; }
  std::cout << "Ransac computed plane: n=" << plane.n.transpose() << " d=" << plane.d << std::endl;
  find_plane = false;
}

void DepthCamera::get_3d_pt(int x, int y, float* out) {
  out[0] = out[1] = out[2] = 0.0f;
}

