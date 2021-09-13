libfreenect2::Registration* registration;
libfreenect2::Frame* undistorted;

void get_3d_point(int x, int y, float* out) {
    float px,py,pz;
    registration->getPointXYZ(undistorted,y,x,px,py,pz);
    out[0] = px; out[1] = py; out[2] = pz;
}

void denoise_depth(libfreenect2::Registration* registration, libfreenect2::Frame* undistorted, float distance, PlaneModel<float>* plane) {

  // try to reduce background noise
  // depth pixels that are set to zero in this loop are _foreground_
  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {

      float px,py,pz;
      registration->getPointXYZ(undistorted,y,x,px,py,pz);
      // areas with corrupt data: be conservative and assume they are foreground
      if (std::isnan(pz) || std::isinf(pz) || pz <= 0)
        ((float*)undistorted->data)[y*dw+x] = 0.0;

      // areas _above_ the plane are definitely foreground
      Eigen::Vector3f point = { px, py, pz };
      if ((plane->n.dot(point) - plane->d) <= -distance*0.01) {
        ((float*)undistorted->data)[y*dw+x] = 0.0;
      }
    }
  }

  // erode depth image, i.e. expand foreground (zero) areas
  cv::Mat tmp(dh,dw,CV_32FC1,undistorted->data);
  dilate(tmp,tmp,getStructuringElement(MORPH_RECT,Size(3,3)));
  erode(tmp,tmp,getStructuringElement(MORPH_RECT,Size(7,5)));
}

void apply_to_color(libfreenect2::Registration* registration, libfreenect2::Frame* undistorted, float distance, PlaneModel<float>* plane, libfreenect2::Frame* rgb, int* color_map) {

  // blank out all remaining color pixels _below_ the plane
  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {

      float px,py,pz;
      registration->getPointXYZ(undistorted,y,x,px,py,pz);
      if (std::isnan(pz) || std::isinf(pz) || pz <= 0) continue;
      Eigen::Vector3f point = { px, py, pz };

      if ((plane->n.dot(point) - plane->d) > -distance*0.01) {
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
