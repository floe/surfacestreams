#ifndef _LIBCAMERA_H_
#define _LIBCAMERA_H_

#include "Camera.h"

#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

#include <libcamera/libcamera.h>

class Libcamera: public Camera {

	public:

		Libcamera(const char* pipe, const char* dev, int cw, int ch);
		~Libcamera();

		void retrieve_frames();
		void remove_background();

	private:

		void request_completed(libcamera::Request* request);

		std::unique_ptr<libcamera::CameraManager> cm;
		std::shared_ptr<libcamera::Camera> camera;
		std::unique_ptr<libcamera::CameraConfiguration> config;
		std::unique_ptr<libcamera::FrameBufferAllocator> allocator;

		std::vector<std::unique_ptr<libcamera::Request>> requests;

		// Synchronization between requestCompleted callback and retrieve_frames()
		std::mutex frame_mutex;
		std::condition_variable frame_ready;
		std::queue<libcamera::Request*> completed_requests;

		// Memory-mapped frame buffers: fd -> (pointer, size)
		struct MappedBuffer {
			void* data;
			size_t size;
		};
		std::map<int, MappedBuffer> mapped_buffers;
};

#endif // _LIBCAMERA_H_
