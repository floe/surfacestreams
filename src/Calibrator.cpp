#include "Calibrator.h"

#include <vector>
#include <iostream>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////
//
// OpenCV stuff
//

#include <Eigen/Core>
#include <opencv2/core/core.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>

using namespace cv;

Mat Calibrator::calcPerspective() {

  Mat result;

  dst.push_back(Point2f( 0, 0));
  dst.push_back(Point2f(tw, 0));
  dst.push_back(Point2f(tw,th));
  dst.push_back(Point2f( 0,th));

  result = getPerspectiveTransform(src,dst);

  src.clear();
  dst.clear();

  return result;
}

void Calibrator::autoPerspective(Mat input) {

  aruco::DetectorParameters detectorParams;
  detectorParams.cornerRefinementMethod = 0; // 0: None, 1: Subpixel, 2: Contour, 3: AprilTag

  aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(aruco::DICT_4X4_50);

  aruco::ArucoDetector detector(dictionary, detectorParams);

  std::vector<int> ids;
  std::vector<std::vector<Point2f> > markers;

  Mat inverted = Scalar::all(255) - input;

  // detect markers and estimate pose
  detector.detectMarkers(inverted, markers, ids);

  std::cout << "\nmarkers:" << std::endl;
  for (unsigned int num = 0; num < ids.size(); num++) {
    int id = ids[num];
    std::cout << "Marker: " << id << " " << markers[num][0] << std::endl;
    if (id > 4) continue;
    // store the first corner of the marker in the respective list
    corners[id].push_back(markers[num][0]);
  }

  // check if all corners have at least 10 samples
  for (int i = 0; i < 4; i++) 
    if (corners[i].size() < 10)
      return;

  std::cout << corners[0] << corners[1] << corners[2] << corners[3] << std::endl;

  for (int i = 0; i < 4; i++) {
    Point2f corner(0,0);
    for (auto pt: corners[i]) corner += pt;
    corner /= (float)corners[i].size();
    corners[i].clear();
    std::cout << corner << std::endl;
    src.push_back(corner);
  }
  pm = calcPerspective();
  std::cout << pm << std::endl;
}


void Calibrator::push_point(float x, float y) {
  src.push_back(Point2f((float)cw*x/(float)tw,(float)ch*y/(float)th));
  if (src.size() == 4) {
    pm = calcPerspective();
    std::cout << "Perspective transform matrix: ";
    for (auto el: cv::Mat_<float>(pm.inv())) std::cout << el << ",";
    std::cout << std::endl;
  }
}

