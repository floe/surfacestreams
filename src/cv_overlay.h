// OpenCV helper function to overlay an image with alpha channel on top of a second image
// based on https://jepsonsblog.blogspot.com/2012/10/overlay-transparent-image-in-opencv.html 
// Assumption: src is RGB-U8, overlay is RGBA-U8

void OverlayImage(cv::Mat* src, cv::Mat* overlay, const cv::Point& location) {

  for (int y = std::max(location.y, 0); y < src->rows; ++y) {

    int fY = y - location.y;
    if (fY >= overlay->rows) break;
  
    for (int x = std::max(location.x, 0); x < src->cols; ++x) {

      int fX = x - location.x;
      if (fX >= overlay->cols) break;
      
      double opacity = ((double)overlay->data[fY * overlay->step + fX * overlay->channels() + 3]) / 255;
      
      for (int c = 0; opacity > 0 && c < src->channels(); ++c) {

        unsigned char overlayPx = overlay->data[fY * overlay->step + fX * overlay->channels() + c];
        unsigned char srcPx = src->data[y * src->step + x * src->channels() + c];
        src->data[y * src->step + src->channels() * x + c] = srcPx * (1.0 - opacity) + overlayPx * opacity;
      }
    }
  }
}
