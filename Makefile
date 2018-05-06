webcam: webcam.cpp common.cpp
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ -I /usr/include/eigen3/ -I include/ $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv)

realsense: realsense.cpp common.cpp
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ -I include/ -lrealsense2

#kinect: libfreenect2
# make -C libfreenect2/build/
# cp libfreenect2/build/bin/Protonect ./kinect

#libfreenect2:
#	git submodule update --init
#	cd libfreenect2; mkdir -p build; cd build
#	cmake .. -DENABLE_CXX11=1

clean:
	-rm webcam realsense kinect
