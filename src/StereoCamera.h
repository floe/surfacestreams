#ifndef _STEREOCAMERA_H_
#define _STEREOCAMERA_H_

#include <Libcamera.h>
#include <DepthCamera.h>

class StereoCamera: virtual public Libcamera, DepthCamera {

	public:

		StereoCamera(const char* pipe, const char* dev, int cw, int ch);
		~StereoCamera();

		void retrieve_frames();
		void release_frames();

	protected:


};

#endif // _STEREOCAMERA_H_
