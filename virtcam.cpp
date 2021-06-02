#include <unistd.h>
#include <iostream>

#include "VirtualCam.h"

#define IN_W 1280
#define IN_H  720

int main(int argc, char* argv[]) {

  char* gstpipe = nullptr;
  if (argc == 2) gstpipe = argv[1];

  VirtualCam* cam = new VirtualCam(gstpipe);

  while (!cam->do_quit) {

    cam->retrieve_frames();

    cam->remove_background();

    cam->send_buffer();
  }

  return 0;
}
