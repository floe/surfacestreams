*void handle_frame() {
  listener.waitForNewFrame(frames);
  libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
  libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
  libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];

  // clip depth to max. 1m
  float* depth_s = ((float*)depth->data);
  float limit = 1000.0;
  __m256 ymm0, ymm1, ymm2, ymm3 = _mm256_broadcast_ss(&limit);
  for (int i = 0; i < (512*424)/8; i++, depth_s+=8) {
    ymm0 = _mm256_load_ps(depth_s);
    ymm1 = _mm256_cmp_ps(ymm0,ymm3,_CMP_LE_OS);
    ymm2 = _mm256_and_ps(ymm0,ymm1);
    _mm256_store_ps(depth_s,ymm2);
    //if (*depth_s > 1000.0) *depth_s = 0;
  }

  registration->apply(rgb,depth,&undistorted,&registered,true,&bigdepth);

  float* depth_p = ((float*)bigdepth.data)+1920;
  unsigned int* rgb_src = (unsigned int*)rgb->data;
  __m256 ymm4, ymm5 = _mm256_broadcast_ss(&limit);
  for (int i = 0; i < (1920*1080)/8; i++, depth_p+=8, rgb_src+=8) {
    ymm0 = _mm256_load_ps(depth_p);
    ymm1 = _mm256_cmp_ps(ymm0,ymm5,_CMP_LE_OS);
    ymm4 = (__m256)_mm256_load_si256((__m256i*)rgb_src);
    ymm2 = _mm256_and_ps(ymm4,ymm1);
    _mm256_store_si256((__m256i*)rgb_src,(__m256i)ymm2);
  }
}
