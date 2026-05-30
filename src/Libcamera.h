#ifndef _LIBCAMERA_H_
#define _LIBCAMERA_H_

#include "Camera.h"

#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

#include <libcamera/libcamera.h>

struct MappedBuffer {
	void* data;
	size_t size;
};

class LibCamWrapper {

	public:

		LibCamWrapper(std::shared_ptr<libcamera::Camera> dev, int _cw, int _ch);
		~LibCamWrapper();

		cv::Mat retrieve_frames();
		void release_frames();

	private:

		int cw,ch;

		// libcamera helper & config objects
		std::shared_ptr<libcamera::Camera> camera;
		std::unique_ptr<libcamera::CameraConfiguration> config;
		libcamera::FrameBufferAllocator* allocator;

		// callback and variable for most recent completed request
		std::vector<std::unique_ptr<libcamera::Request>> requests;
		void request_completed(libcamera::Request* request);
		libcamera::Request* request;

		// synchronization between request_completed and retrieve_frames
		std::mutex frame_mutex;
		std::condition_variable frame_ready;
		std::queue<libcamera::Request*> completed_requests;

		// memory-mapped frame buffers: fd -> (pointer, size)
		std::map<int, MappedBuffer> mapped_buffers;
};

class Libcamera: public Camera {

	public:

		Libcamera(const char* pipe, const char* dev, int cw, int ch);
		~Libcamera();

		void retrieve_frames();
		void release_frames();

	private:

		libcamera::CameraManager* cm;
		LibCamWrapper* cam[2];

};

#endif // _LIBCAMERA_H_
