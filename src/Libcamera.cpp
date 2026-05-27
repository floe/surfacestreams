#include "Libcamera.h"

#include <iostream>
#include <stdexcept>
#include <sys/mman.h>

#include <libcamera/formats.h>

Libcamera::Libcamera(const char* pipe, const char* dev, int _cw, int _ch):
  Camera(pipe, "BGR", _cw, _ch)
{
  cm = new libcamera::CameraManager();
  if (cm->start() < 0) throw std::runtime_error("Failed to start CameraManager.");

  // select camera by name first, try by index if not found
  auto cameras = cm->cameras();
  unsigned int idx = dev[0] - '0';
  camera = cm->get(dev);

  if (!camera) camera = (idx < cameras.size()) ? cameras[idx] : nullptr;
  if (!camera) throw std::runtime_error(std::string("Failed to find camera: ") + dev);
  if (camera->acquire() < 0) throw std::runtime_error("Failed to acquire camera.");

  // generate, validate, apply a VideoRecording configuration
  config = camera->generateConfiguration({ libcamera::StreamRole::VideoRecording });
  if (!config) throw std::runtime_error("Failed to create camera configuration.");

  libcamera::StreamConfiguration& streamcfg = config->at(0);
  streamcfg.pixelFormat = libcamera::formats::RGB888; // FIXME: why is color order flipped, should be BGR888?
  streamcfg.size = { (unsigned int)cw, (unsigned int)ch };

  libcamera::CameraConfiguration::Status status = config->validate();
  if (status != libcamera::CameraConfiguration::Valid) throw std::runtime_error("Failed to validate camera configuration.");
  if (camera->configure(config.get()) < 0) throw std::runtime_error("Failed to apply camera configuration.");

  // allocate frame buffers
  libcamera::Stream* stream = streamcfg.stream();
  allocator = new libcamera::FrameBufferAllocator(camera);
  if (allocator->allocate(stream) < 0) throw std::runtime_error("Failed to allocate frame buffers.");

  // for each frame buffer:
  for (const std::unique_ptr<libcamera::FrameBuffer>& buf: allocator->buffers(stream)) {

    // memory-map first buffer plane (TODO: wouldn't work with multi-plane buffers)
    const libcamera::FrameBuffer::Plane& plane = buf->planes()[0];
    int fd = plane.fd.get();
    // if (mapped_buffers.count(fd) > 0) continue; // already mapped (multi-plane may share fd)
    void* data = mmap(nullptr, plane.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) throw std::runtime_error("Failed to mmap frame buffer.");
    mapped_buffers[fd] = { data, plane.length };

    // create a streaming request and pre-queue
    std::unique_ptr<libcamera::Request> req = camera->createRequest();
    if (!req) throw std::runtime_error("Failed to create streaming request.");
    if (req->addBuffer(stream, buf.get()) < 0) throw std::runtime_error("Failed to add buffer to streaming request.");
    requests.push_back(std::move(req));
  }

  // cf. https://stackoverflow.com/a/78632543/
  libcamera::ControlList camcontrols;
  camcontrols.set(libcamera::controls::FrameDurationLimits, libcamera::Span<const std::int64_t, 2>({60000, 70000}));

  // connect the requestCompleted signal and start
  camera->requestCompleted.connect(this, &Libcamera::request_completed);
  if (camera->start(&camcontrols) < 0) throw std::runtime_error("Failed to start camera.");

  // enqueue all streaming requests
  for (auto& req: requests) if (camera->queueRequest(req.get()) < 0) throw std::runtime_error("Failed to queue streaming request.");
}

Libcamera::~Libcamera() {
  camera->stop();
  //allocator->free(stream);
  delete allocator;

  for (auto& [fd, mb] : mapped_buffers)
    munmap(mb.data, mb.size);

  camera->release();
  camera.reset();

  cm->stop();
  delete cm;
}

// callback for completed streaming requests
void Libcamera::request_completed(libcamera::Request* request) {
  if (request->status() == libcamera::Request::RequestCancelled) return;
  std::lock_guard<std::mutex> lock(frame_mutex);
  completed_requests.push(request);
  frame_ready.notify_all();
}

void Libcamera::retrieve_frames() {
  // wait for a completed request (block scope for lock)
  {
    std::unique_lock<std::mutex> lock(frame_mutex);
    while (completed_requests.empty()) frame_ready.wait(lock);
    request = completed_requests.front();
    completed_requests.pop();
  }

  // get the first plane of the first buffer
  libcamera::Stream* stream = config->at(0).stream();
  libcamera::FrameBuffer* buf = request->buffers().at(stream);
  const libcamera::FrameBuffer::Plane& plane = buf->planes()[0];

  void* data = mapped_buffers.at(plane.fd.get()).data;
  int stride = config->at(0).stride;

  // wrap the mapped data in a cv::Mat and assign to result
  input = cv::Mat(ch, cw, CV_8UC3, data, stride);
}

void Libcamera::release_frames() {
  // requeue the finished request for the next frame
  request->reuse(libcamera::Request::ReuseBuffers);
  camera->queueRequest(request);
}
