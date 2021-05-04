// based on:
// https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/examples/viewer/opengl/main.cpp
// based on:
// https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/examples/transformation/main.cpp
// tested with: libk4a1.4-1.4.1

#include "KinectAzure.h"
#include "timer.h"
#include <iostream>

// to enable glog logging, set LOG = 1
// CAVEAT: If logging enabled, build project
//         using using CMakeLists to pull in
//         glog dependencies
//
#define LOG 0
#if LOG == 1
#include "logger.h"
#endif

int main(int argc, char* argv[])
{
#if LOG == 1
    logger(argc, argv);
#endif

    const char* gstpipe = nullptr;
    if (argc >= 2)
        gstpipe = argv[1];

    KinectAzure cam(gstpipe);

    while (!cam.do_quit) {

        cam.retrieve_frames();

        if (cam.find_plane) {
            Timer timer;
            cam.ransac_plane();
#if LOG == 1
            LOG(INFO) << "-- segmented plane in " << timer.getDuration()
                      << " ms";
#endif
            cam.find_plane = false;
        }

        cam.remove_background();

        cam.send_buffer();
    }
}
