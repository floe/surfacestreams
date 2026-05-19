#include "TuioOverlay.h"

#include <vector>
#include <iostream>
#include <unistd.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "cv_overlay.h"

using namespace cv;

TuioOverlay::TuioOverlay(int _tw, int _th) {

  tw = _tw;
  th = _th;

  // TUIO handling
  client = new TUIO::TuioClient();
  client->connect();

  std::vector<cv::String> filenames;
  cv::glob("assets/tuio-object-*.png",filenames);
  for (auto file: filenames) {
    std::cout << "Loading TUIO object image " << file << std::endl;
    size_t dashpos = file.rfind("-");
    size_t dotpos = file.rfind(".");
    cv::String mynum = file.substr(dashpos+1,dotpos-dashpos-1);
    pix[atoi(mynum.c_str())] = cv::imread(file,cv::IMREAD_UNCHANGED);
  }
  pix[-1] = cv::imread("assets/tuio-cursor.png",cv::IMREAD_UNCHANGED);
}

void TuioOverlay::process(Mat* output) {

  // Overlay TuioCursors as "touchpoint"
  std::list<TUIO::TuioCursor*> cursors = client->getTuioCursors();
  for (auto cursor: cursors) {
    TUIO::TuioPoint foo = cursor->getPosition();
    //std::cout << cursor->getSessionID() << " " << cursor->getCursorID() << " " << foo.getX() << " " << foo.getY() << std::endl;
    Point2f point(foo.getX()*tw-(pix[-1].cols/2),foo.getY()*th-(pix[-1].rows/2));
    OverlayImage(output,&(pix[-1]),point);
  }

  // Overlay TuioObject images (if configured)
  std::list<TUIO::TuioObject*> objects = client->getTuioObjects();
  for (auto object: objects) {

    TUIO::TuioPoint foo = object->getPosition();
    float angle = -360.0*(object->getAngle()/(2.0*M_PI));
    int symid = object->getSymbolID();

    if (pix.find(symid) == pix.end()) continue;
    cv::Mat temp = pix[symid];

    Point2f center(temp.cols/2,temp.rows/2);
    cv::Mat target(temp.cols,temp.rows,CV_8UC4);
    cv::Mat rotation = cv::getRotationMatrix2D(center,angle,1.0);
    cv::warpAffine(temp,target,rotation,target.size());

    Point2f point(foo.getX()*tw-(target.cols/2),foo.getY()*th-(target.rows/2));
    OverlayImage(output,&target,point);
  }
}
