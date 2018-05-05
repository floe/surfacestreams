/* 
 
int dw = 512, dh = 424;
int cw = 1920, ch = 1080;

// use RANSAC to compute a plane out of sparse point cloud
PlaneModel<float> find_plane(Registration* registration, libfreenect2::Frame* undistorted, float distance) {

  std::vector<Eigen::Vector3f> points;

  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {
      float px,py,pz;
      registration->getPointXYZ(&undistorted,y,x,px,py,pz);
      if (std::isnan(pz) || std::isinf(pz) || pz <= 0) continue;
      Eigen::Vector3f point = { px, py, pz };
      points.push_back( point );
    }
  }

  std::cout << "3D point count: " << points.size() << std::endl;
  PlaneModel<float> plane = ransac<PlaneModel<float>>( points, distance*0.01, 200 );
  if (plane.d < 0.0) { plane.d = -plane.d; plane.n = -plane.n; }
  std::cout << "Ransac computed plane: n=" << plane.n.transpose() << " d=" << plane.d << std::endl;

  return plane;
}

void denoise_depth(Registration* registration, libfreenect2::Frame* undistorted, float distance, PlaneModel<float>* plane) {

  // try to reduce background noise
  // depth pixels that are set to zero in this loop are _foreground_
  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {

      float px,py,pz;
      registration->getPointXYZ(&undistorted,y,x,px,py,pz);
      // areas with corrupt data: be conservative and assume they are foreground
      if (std::isnan(pz) || std::isinf(pz) || pz <= 0)
        ((float*)undistorted.data)[y*dw+x] = 0.0;

      // areas _above_ the plane are definitely foreground
      Eigen::Vector3f point = { px, py, pz };
      if ((plane.n.dot(point) - plane.d) <= -distance*0.01) {
        ((float*)undistorted.data)[y*dw+x] = 0.0;
      }
    }
  }

  // erode depth image, i.e. expand foreground (zero) areas
  cv::Mat tmp(dh,dw,CV_32FC1,undistorted.data);
  dilate(tmp,tmp,getStructuringElement(MORPH_RECT,Size(3,3)));
  erode(tmp,tmp,getStructuringElement(MORPH_RECT,Size(7,5)));
}

void apply_to_color(Registration* registration, libfreenect2::Frame* undistorted, float distance, PlaneModel<float>* plane, libfreenect2::Frame* rgb, uint32_t* color_map) {

  // blank out all remaining color pixels _below_ the plane
  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {

      float px,py,pz;
      registration->getPointXYZ(&undistorted,y,x,px,py,pz);
      if (std::isnan(pz) || std::isinf(pz) || pz <= 0) continue;
      Eigen::Vector3f point = { px, py, pz };

      if (filter) if ((plane.n.dot(point) - plane.d) > -distance*0.01) {
        int index = color_map[y*dw+x];

        // creates rectangular patch of size 5x3
        if (index < cw+2 || index >= cw*(ch-1)-2) continue;
        for (int ty = -cw; ty <= cw; ty += cw)
          for (int tx = -2; tx <= 2; tx++)
            ((uint32_t*)rgb->data)[index+ty+tx] = 0x0000FF00;
      }
    }
  }
}

    libfreenect2::Frame *newrgb = new libfreenect2::Frame(1280, 720, 4);

    Mat input(1080,1920,CV_8UC4,rgb->data);
    Mat output(720,1280,CV_8UC4,newrgb->data);
    warpPerspective(input,output,pm,output.size(),INTER_NEAREST);

    prepare_buffer((GstAppSrc*)appsrc,newrgb);
    g_main_context_iteration(g_main_context_default(),FALSE);

*/
