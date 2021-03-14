CPPFLAGS=-std=c++11 -O3 -Wall -ggdb -I /usr/include/eigen3/ -I include/ $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4)

all: webcam sur40 realsense kinect k4a

%.o: %.cpp %.h
	g++ -c -o $@ $< ${CPPFLAGS}

webcam: webcam.cpp Camera.cpp
	g++ -o $@ $^ ${CPPFLAGS}

sur40: webcam.cpp Camera.cpp
	g++ -o $@ $^ -DSUR40 ${CPPFLAGS}

realsense: realsense.cpp common.cpp
	g++ -o $@ $^ ${CPPFLAGS} -lrealsense2

k4a: k4a.cpp common.cpp
	g++ -o $@ $^ ${CPPFLAGS} -lk4a

kinect: libfreenect2 libfreenect2/examples/Protonect.cpp
	make -C libfreenect2/build/
	cp libfreenect2/build/bin/Protonect ./kinect

libfreenect2: libfreenect2/CMakeLists.txt
	git submodule update --init
	cd libfreenect2 && mkdir -p build
	cd libfreenect2/build && cmake .. -DENABLE_CXX11=1
	touch libfreenect2/

clean:
	-rm webcam sur40 realsense kinect k4a
