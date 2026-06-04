#include <StereoCamera.h>

StereoCamera::StereoCamera(const char* pipe, const char* dev, int cw, int ch):
  Camera(pipe,"BGR",cw,ch),
  Libcamera(pipe,"0",cw,ch),
  DepthCamera(pipe,"BGR",cw,ch)
{
  cam[1] = new LibCamWrapper(cameras[1],cw,ch);
}

void StereoCamera::retrieve_frames() { }
void StereoCamera::release_frames() { }
