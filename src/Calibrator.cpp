#include "Calibrator.h"

#include <vector>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

Calibrator::Calibrator(int _cw, int _ch, int _tw, int _th ) {
  cw = _cw; ch = _ch; tw = _tw; th = _th;
  im = (Mat_<float>(3,3) << (float)tw/(float)cw, 0, 0, 0, (float)th/(float)ch, 0, 0, 0, 1 );
  pm = im;

  aruco::DetectorParameters detectorParams;
  detectorParams.cornerRefinementMethod = 0; // 0: None, 1: Subpixel, 2: Contour, 3: AprilTag

  aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(aruco::DICT_4X4_50);

  detector = aruco::ArucoDetector(dictionary, detectorParams);
}

void Calibrator::reset() { pm = im; }
void Calibrator::draw(Mat* output) { for (Point2f point: src) cv::rectangle(*output,point,point,Scalar(0,0,255),10); }

Mat Calibrator::calcPerspective() {

  Mat result;
  dst.clear();

  dst.push_back(Point2f( 0, 0));
  dst.push_back(Point2f(tw, 0));
  dst.push_back(Point2f(tw,th));
  dst.push_back(Point2f( 0,th));

  result = getPerspectiveTransform(src,dst);

  src.clear();
  dst.clear();

  return result;
}

bool Calibrator::autoPerspective(Mat input) {

  std::vector<int> ids;
  std::vector<std::vector<Point2f> > markers;

  Mat inverted = Scalar::all(255) - input;

  // detect markers and estimate pose
  detector.detectMarkers(inverted, markers, ids);
  aruco::drawDetectedMarkers(input, markers, ids);

  std::ostringstream message; message << "Detecting markers (need > 10 per corner): ";
  message << corners[0].size() << " " << corners[1].size() << " " << corners[2].size() << " " << corners[3].size();
  cv::putText(input,message.str().c_str(),Point2f(30,50),cv::FONT_HERSHEY_SIMPLEX,1.0,Scalar(1,1,1));

  // walk through all detected markers (id in ids, corner points in markers)
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
      return true; // continue sampling

  src.clear();

  // FIXME: needs to restart if variance is too high
  for (int i = 0; i < 4; i++) {
    Point2f corner(0,0);
    for (auto pt: corners[i]) corner += pt;
    corner /= (float)corners[i].size();
    corners[i].clear();
    std::cout << corner << std::endl;
    src.push_back(corner);
  }
  pm = calcPerspective();

  return false;
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

