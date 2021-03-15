#include <unistd.h>
#include <iostream>

#include "V4L2.h"
#include "SUR40.h"

#define IN_W 1280
#define IN_H  720

int get_v4l_devnum(const char* path) {
  char buf[128]; if (!path) path = "/dev/video0";
  int num,res = readlink(path,buf,sizeof(buf));
  if (res == -1) { strncpy(buf,path,sizeof(buf)); res = strlen(buf); }
  num = (int)(buf[res-1] - '0');
  std::cout << "path " << path << " maps to " << buf << ", devnum " << num << std::endl;
  return num;
}

int main(int argc, char* argv[]) {

  char* gstpipe = nullptr;
  if (argc > 2) gstpipe = argv[2];

  Camera* cam;

  if (std::string(argv[0]).find("sur40") == std::string::npos)
    cam = new V4L2(gstpipe,get_v4l_devnum(argv[1]),IN_W,IN_H);
  else
    cam = new SUR40(gstpipe,get_v4l_devnum(argv[1]));

  while (!cam->do_quit) {

    cam->retrieve_frames();

    cam->remove_background();

    cam->send_buffer();
  }

  return 0;
}
