#include "Libcamera.h"

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>

#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>

#include <libcamera/formats.h>

// DMA heap, modeled on rpicam-apps/core/rpicam_app.cpp
static const std::vector<const char*> heap_names = {
  "/dev/dma_heap/vidbuf_cached",
  "/dev/dma_heap/linux,cma",
  "/dev/dma_heap/system"
};

DmaHeap::DmaHeap() {
  for (const char* name: heap_names) {
    int fd = open(name, O_RDWR | O_CLOEXEC, 0);
    if (fd > 0) { heap_fd = libcamera::UniqueFD(fd); return; }
  }
  throw std::runtime_error("No suitable DMA heap found.");
}

libcamera::SharedFD DmaHeap::allocate(const std::string name, std::size_t size) {

  struct dma_heap_allocation_data alloc = {
    .len = size,
    .fd_flags = O_CLOEXEC | O_RDWR,
  };

  if (ioctl(heap_fd.get(), DMA_HEAP_IOCTL_ALLOC, &alloc) < 0)
    throw std::runtime_error("Failed to allocate DMA heap buffer.");

  if (ioctl(alloc.fd, DMA_BUF_SET_NAME, name.c_str()) < 0)
    throw std::runtime_error("Failed to set name for DMA heap buffer.");

  return libcamera::SharedFD(alloc.fd);
}

Libcamera::Libcamera(const char* pipe, const char* dev, int _cw, int _ch):
  Camera(pipe, "BGR", _cw, _ch)
{
  cm = new libcamera::CameraManager();
  if (cm->start() < 0) throw std::runtime_error("Failed to start CameraManager.");

  // select camera by name first, try by index if not found
  cameras = cm->cameras();
  unsigned int idx = dev[0] - '0';
  auto camera = cm->get(dev);

  if (!camera) camera = (idx < cameras.size()) ? cameras[idx] : nullptr;
  if (!camera) throw std::runtime_error(std::string("Failed to find camera: ") + dev);

  cam[0] = new LibCamWrapper(camera,_cw,_ch);
}

LibCamWrapper::LibCamWrapper(std::shared_ptr<libcamera::Camera> dev, int _cw, int _ch) {

  camera = dev;
  cw = _cw;
  ch = _ch;

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
  for (unsigned int i = 0; i < streamcfg.bufferCount; i++) {

    // create a single-plane framebuffer via DMA heap
    std::vector<libcamera::FrameBuffer::Plane> planes(1);
    planes[0].fd = dma_heap.allocate("surfacestreams-"+std::to_string(i), streamcfg.frameSize);
    planes[0].offset = 0;
    planes[0].length = streamcfg.frameSize;

    framebuffers.push_back(std::make_unique<libcamera::FrameBuffer>(planes));
    std::unique_ptr<libcamera::FrameBuffer>& buf = framebuffers.back();

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
  libcamera::Span<const std::int64_t, 2> span({60000, 70000});
  camcontrols.set(libcamera::controls::FrameDurationLimits, span);
  // req->controls().set(libcamera::controls::FrameDurationLimits, span);

  // connect the requestCompleted signal and start
  camera->requestCompleted.connect(this, &LibCamWrapper::request_completed);
  if (camera->start(0) < 0) throw std::runtime_error("Failed to start camera.");

  // enqueue all streaming requests
  for (auto& req: requests) if (camera->queueRequest(req.get()) < 0) throw std::runtime_error("Failed to queue streaming request.");
}

LibCamWrapper::~LibCamWrapper() {
  camera->stop();

  for (auto& [fd, mb] : mapped_buffers)
    munmap(mb.data, mb.size);

  requests.clear();
  framebuffers.clear();

  camera->release();
  camera.reset();
}

Libcamera::~Libcamera() {
  delete cam[0];
  delete cam[1];
  cm->stop();
  delete cm;
}

// callback for completed streaming requests
void LibCamWrapper::request_completed(libcamera::Request* request) {
  if (request->status() == libcamera::Request::RequestCancelled) return;
  std::lock_guard<std::mutex> lock(frame_mutex);
  completed_requests.push(request);
  frame_ready.notify_all();
}

cv::Mat LibCamWrapper::retrieve_frames() {
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

 // wrap the mapped data in a cv::Mat and return
  void* data = mapped_buffers.at(plane.fd.get()).data;
  int stride = config->at(0).stride;
  return cv::Mat(ch, cw, CV_8UC3, data, stride);
}

void LibCamWrapper::release_frames() {
  // requeue the finished request for the next frame
  request->reuse(libcamera::Request::ReuseBuffers);
  camera->queueRequest(request);
}

void Libcamera::retrieve_frames() {
  input = cam[0]->retrieve_frames();
}

void Libcamera::release_frames() {
  for (auto& c: cam) if (c) c->release_frames();
}
