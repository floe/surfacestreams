all: webcam sur40 realsense kinect k4a

webcam: webcam.cpp common.cpp
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ -I /usr/include/eigen3/ -I include/ $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4)

sur40: webcam.cpp common.cpp
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ -DSUR40 -I /usr/include/eigen3/ -I include/ $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4)

realsense: realsense.cpp common.cpp
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ -I /usr/include/eigen3/ -I include/ -lrealsense2 $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4)

k4a: k4a.cpp common.cpp
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ -I /usr/include/eigen3/ -I include/ -lk4a $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4)

kinect: libfreenect2 libfreenect2/examples/Protonect.cpp
	make -C libfreenect2/build/
	cp libfreenect2/build/bin/Protonect ./kinect

libfreenect2: libfreenect2/CMakeLists.txt
	git submodule update --init
	cd libfreenect2 && mkdir -p build
	cd libfreenect2/build && cmake .. -DENABLE_CXX11=1
	touch libfreenect2/

clean:
	-rm webcam realsense kinect
