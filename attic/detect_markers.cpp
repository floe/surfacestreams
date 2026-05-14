// build with: g++ -Wall -g -ggdb detect_markers.cpp $(pkg-config --cflags --libs opencv4) -o detect_markers

#include <opencv2/highgui.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <iostream>

using namespace std;
using namespace cv;

const string refineMethods[4] = {
    "None",
    "Subpixel",
    "Contour",
    "AprilTag"
};

int main(int argc, char *argv[]) {

    bool showRejected = false;

    aruco::DetectorParameters detectorParams;
    aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(aruco::DICT_4X4_50);

    detectorParams.cornerRefinementMethod = 0;

    std::cout << "Corner refinement method: " << refineMethods[detectorParams.cornerRefinementMethod] << std::endl;

    //! [aruco_detect_markers]
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);
    cv::VideoCapture inputVideo;
    int waitTime;
    inputVideo.open("libcamerasrc ! video/x-raw,format=I420,width=1280,height=720,framerate=15/1 ! appsink",CAP_GSTREAMER);
    waitTime = 10;

    double totalTime = 0;
    int totalIterations = 0;

    while(inputVideo.grab()) {
	cv::Mat raw, image, imageCopy;
        inputVideo.retrieve(raw);
        image = Scalar::all(255) - raw;

        double tick = (double)getTickCount();

        //! [aruco_pose_estimation3]
        vector<int> ids;
        vector<vector<Point2f> > corners, rejected;

        // detect markers and estimate pose
        detector.detectMarkers(image, corners, ids, rejected);

        double currentTime = ((double)getTickCount() - tick) / getTickFrequency();
        totalTime += currentTime;
        totalIterations++;
        if(totalIterations % 30 == 0) {
            cout << "Detection Time = " << currentTime * 1000 << " ms "
                 << "(Mean = " << 1000 * totalTime / double(totalIterations) << " ms)" << endl;
        }
        // draw results
        image.copyTo(imageCopy);
        if(!ids.empty()) {
            cv::aruco::drawDetectedMarkers(imageCopy, corners, ids);
        }

        if(showRejected && !rejected.empty())
            cv::aruco::drawDetectedMarkers(imageCopy, rejected, noArray(), Scalar(100, 0, 255));

        if(totalIterations % 30 == 0) imshow("out", imageCopy);
        char key = (char)waitKey(waitTime);
        if(key == 27) break;
    }
    //! [aruco_detect_markers]
    return 0;
}
