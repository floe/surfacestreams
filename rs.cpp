#include <iostream>
#include "Realsense.h"

int main( int argc, char* argv [] ) {

	const char* gstpipe = nullptr;
	if (argc >= 2) gstpipe = argv[1];

	Realsense cam(gstpipe);

	while (!cam.do_quit) {

		cam.retrieve_frames();

		if (cam.find_plane) {
			cam.ransac_plane();
			cam.find_plane = false;
		}

		cam.remove_background();

		cam.send_buffer();
	}
}
