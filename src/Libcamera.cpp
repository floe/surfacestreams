// Based on:
//   * https://libcamera.org/api-html/index.html
//   * Camera and V4L2 classes in this project
// Tested with:
//   * libcamera 0.2.0

#include "Libcamera.h"

#include <iostream>
#include <stdexcept>
#include <sys/mman.h>

#include <libcamera/formats.h>

Libcamera::Libcamera(const char* pipe, const char* dev, int _cw, int _ch):
  Camera(pipe, "BGR", _cw, _ch)
{
  cm = std::make_unique<libcamera::CameraManager>();
  if (cm->start() < 0)
    throw std::runtime_error("Failed to start libcamera CameraManager.");

  // Select camera by index if dev is a digit, otherwise by ID string
  std::shared_ptr<libcamera::Camera> selected;
  auto cameras = cm->cameras();
  if (cameras.empty())
    throw std::runtime_error("No libcamera cameras found.");

  // If device is a single digit treat it as a 0-based index
  std::string devstr(dev);
  bool is_index = (devstr.size() == 1 && std::isdigit(devstr[0]));
  if (is_index) {
    unsigned int idx = devstr[0] - '0';
    if (idx >= cameras.size())
      throw std::runtime_error("Camera index out of range.");
    selected = cameras[idx];
  } else {
    selected = cm->get(devstr);
    if (!selected)
      throw std::runtime_error(std::string("Camera not found: ") + devstr);
  }

  camera = selected;
  if (camera->acquire() < 0)
    throw std::runtime_error("Failed to acquire libcamera camera.");

  // Generate a VideoRecording configuration with one stream
  config = camera->generateConfiguration({ libcamera::StreamRole::VideoRecording });
  if (!config)
    throw std::runtime_error("Failed to generate libcamera camera configuration.");

  libcamera::StreamConfiguration& scfg = config->at(0);
  scfg.pixelFormat = libcamera::formats::BGR888;
  scfg.size = { (unsigned int)cw, (unsigned int)ch };

  libcamera::CameraConfiguration::Status status = config->validate();
  if (status == libcamera::CameraConfiguration::Invalid)
    throw std::runtime_error("libcamera configuration is invalid.");
  if (status == libcamera::CameraConfiguration::Adjusted)
    std::cout << "libcamera configuration was adjusted: "
              << scfg.size.width << "x" << scfg.size.height
              << " " << scfg.pixelFormat.toString() << std::endl;

  if (camera->configure(config.get()) < 0)
    throw std::runtime_error("Failed to configure libcamera camera.");

  // Allocate frame buffers
  libcamera::Stream* stream = scfg.stream();
  allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
  if (allocator->allocate(stream) < 0)
    throw std::runtime_error("Failed to allocate libcamera frame buffers.");

  // Memory-map each buffer plane
  for (const std::unique_ptr<libcamera::FrameBuffer>& buf : allocator->buffers(stream)) {
    for (const libcamera::FrameBuffer::Plane& plane : buf->planes()) {
      int fd = plane.fd.get();
      if (mapped_buffers.count(fd)) continue; // already mapped (multi-plane may share fd)
      void* data = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, fd, 0);
      if (data == MAP_FAILED)
        throw std::runtime_error("Failed to mmap libcamera frame buffer.");
      mapped_buffers[fd] = { data, plane.length };
    }
  }

  // Create one Request per buffer and pre-queue all of them
  for (const std::unique_ptr<libcamera::FrameBuffer>& buf : allocator->buffers(stream)) {
    std::unique_ptr<libcamera::Request> req = camera->createRequest();
    if (!req)
      throw std::runtime_error("Failed to create libcamera request.");
    if (req->addBuffer(stream, buf.get()) < 0)
      throw std::runtime_error("Failed to add buffer to libcamera request.");
    requests.push_back(std::move(req));
  }

  // Connect the requestCompleted signal
  camera->requestCompleted.connect(this, &Libcamera::request_completed);

  if (camera->start() < 0)
    throw std::runtime_error("Failed to start libcamera camera.");

  for (auto& req : requests) {
    if (camera->queueRequest(req.get()) < 0)
      throw std::runtime_error("Failed to queue libcamera request.");
  }
}

Libcamera::~Libcamera() {
  camera->stop();
  allocator.reset();

  for (auto& [fd, mb] : mapped_buffers)
    munmap(mb.data, mb.size);

  camera->release();
  cm->stop();
}

void Libcamera::request_completed(libcamera::Request* request) {
  if (request->status() == libcamera::Request::RequestCancelled)
    return;
  std::lock_guard<std::mutex> lock(frame_mutex);
  completed_requests.push(request);
  frame_ready.notify_one();
}

void Libcamera::retrieve_frames() {
  // Wait for a completed request
  libcamera::Request* request;
  {
    std::unique_lock<std::mutex> lock(frame_mutex);
    frame_ready.wait(lock, [this]{ return !completed_requests.empty(); });
    request = completed_requests.front();
    completed_requests.pop();
  }

  // Copy the first plane of the first buffer into input
  libcamera::Stream* stream = config->at(0).stream();
  libcamera::FrameBuffer* buf = request->buffers().at(stream);
  const libcamera::FrameBuffer::Plane& plane = buf->planes()[0];

  void* data = mapped_buffers.at(plane.fd.get()).data;
  int stride = config->at(0).stride;

  // Wrap the mapped data in a cv::Mat (no copy) then clone into input
  cv::Mat frame(ch, cw, CV_8UC3, data, stride);
  input = frame.clone();

  // Re-queue the request for the next frame
  request->reuse(libcamera::Request::ReuseBuffers);
  camera->queueRequest(request);
}

// courtesy of https://www.geeksforgeeks.org/program-change-rgb-color-model-hsv-color-model/
static void BGRtoHSV_lc(uint8_t b, uint8_t g, uint8_t r, double* out) {

  double cmax = std::max(r, std::max(g, b));
  double cmin = std::min(r, std::min(g, b));
  double diff = cmax - cmin;

  double s = (cmax == 0) ? 0.0 : (diff / cmax);
  double v = cmax / 255.0;

  out[1] = s;
  out[2] = v;
}

void Libcamera::remove_background() {

  uint8_t* in_data = (uint8_t*)input.data;

  if (in_data) for (int i = 0; i < input.rows*input.cols*3; i+=3) {

    uint8_t r = in_data[i+0];
    uint8_t g = in_data[i+1];
    uint8_t b = in_data[i+2];

    double hsv[3]; BGRtoHSV_lc(b,g,r,hsv);

    // FIXME: hardcoded threshold
    if (hsv[1] < 0.3 && hsv[2] > 0.7) {
      in_data[i+0] = 0;
      in_data[i+1] = 0xFF;
      in_data[i+2] = 0;
    }
  }
}
