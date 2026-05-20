#ifndef _CALIBRATOR_H_
#define _CALIBRATOR_H_

#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>

class Calibrator {

	public:

		Calibrator(int _cw, int _ch, int _tw, int _th);

		void push_point(float x, float y);

		bool autoPerspective(cv::Mat input);

		void reset();
		void draw(cv::Mat* output);

		cv::Mat pm; // perspective matrix

	protected:

		cv::Mat im; // identity matrix

		int cw, ch; // captured image size
		int tw, th; // final transmitted image size

		cv::Mat calcPerspective();

		std::vector<cv::Point2f> src;
		std::vector<cv::Point2f> dst;

		std::vector<cv::Point2f> corners[4];
		cv::aruco::ArucoDetector detector;
};

#endif // _CALIBRATOR_H_
