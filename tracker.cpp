#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

std::vector< std::vector<cv::KeyPoint> > templates_k;
std::vector< cv::Mat > templates_d;
std::vector< cv::Mat > templates;

Ptr<cv::Feature2D> orb;
Ptr<cv::DescriptorMatcher> matcher;
int MAX_FEATURES = 500;

void init_objects(std::vector<std::string> files) {
  
  orb = ORB::create(MAX_FEATURES);
  matcher = DescriptorMatcher::create("BruteForce-Hamming");
  //matcher = cv::FlannBasedMatcher(cv::makePtr<cv::flann::LshIndexParams>(12, 20, 2)); 
  
  for (auto file: files) {

    Mat gray = imread(file,CV_LOAD_IMAGE_GRAYSCALE);

    std::vector<cv::KeyPoint> kp;
    Mat dp;

    orb->detectAndCompute(gray, Mat(), kp, dp);

    templates_k.push_back(kp);
    templates_d.push_back(dp);
    templates.push_back(gray);
  }
}

void track_objects(cv::Mat input) {

  std::vector<DMatch> matches;
  std::vector<KeyPoint> keypoints;
  Mat descriptors;

  Mat gray; cvtColor(input,gray,CV_BGR2GRAY);
  orb->detectAndCompute(gray, Mat(), keypoints, descriptors);

  for (unsigned int i = 0; i < templates_d.size(); i++) {
    std::vector<KeyPoint>& kpt = templates_k[i];
    cv::Mat desc_t = templates_d[i];

    // only single result here
    matcher->match(descriptors,desc_t,matches); // queryDesc, trainDesc, matches

    std::vector<DMatch> good;
    std::vector<Point2f> p1,p2;
    double mindist = INT_MAX, maxdist = 0;
    for (auto match: matches) {
      if (match.distance < mindist) mindist = match.distance;
      if (match.distance > maxdist) maxdist = match.distance;
    }
    std::cout << "min: " << mindist << " max: " << maxdist << std::endl;
    double threshold = mindist + (maxdist - mindist) * 0.2;

    for (auto match: matches) 
      if (match.distance < threshold && match.distance < 50) { // FIXME hardcoded magic value
        //good.push_back( keypoints[ match.queryIdx] );
        good.push_back( match );
        p1.push_back( keypoints[ match.queryIdx ].pt );
        p2.push_back( kpt[ match.trainIdx ].pt );
      }

    /*Mat newImg;
    drawMatches(templates[i],kpt,input,keypoints,good,newImg);
    imshow("foo",newImg);
    cvWaitKey(1);*/

    if (p1.size() < 4 || p2.size() < 4) continue;
    Mat h = findHomography(p2,p1,RANSAC);

    std::vector<Point2f> obj_corners(4);
    obj_corners[0] = Point2f(0, 0);
    obj_corners[1] = Point2f( 550, 0 );
    obj_corners[2] = Point2f( 550, 400);
    obj_corners[3] = Point2f( 0, 400);
    std::vector<Point2f> scene_corners(4);

    if (h.empty()) continue;
    perspectiveTransform( obj_corners, scene_corners, h);

    line( input, scene_corners[0], scene_corners[1], Scalar(0, 255, 0), 4 );
    line( input, scene_corners[1], scene_corners[2], Scalar(0, 255, 0), 4 );
    line( input, scene_corners[2], scene_corners[3], Scalar(0, 255, 0), 4 );
    line( input, scene_corners[3], scene_corners[0], Scalar(0, 255, 0), 4 );

  }

}

