TARGETS=webcam k4a realsense #kinect
LIBS=gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 glib-2.0 opencv4

CCFLAGS=-std=c++11 -O3 -Wall -ggdb -pg -I /usr/include/eigen3/ -I include/ $(shell pkg-config --cflags ${LIBS})
LDFLAGS=-std=c++11 -O3 -Wall -ggdb -pg $(shell pkg-config --libs ${LIBS})

all: ${TARGETS}

%.o: %.cpp
	g++ -c -o $@ $< ${CCFLAGS}

webcam: webcam.o Camera.o V4L2.o SUR40.o VirtualCam.o
	g++ -o $@ $^ ${LDFLAGS}

realsense: rs.o Camera.o Realsense.o
	g++ -o $@ $^ ${LDFLAGS} -lrealsense2

k4a: k4a.o Camera.o KinectAzure.o
	g++ -o $@ $^ ${LDFLAGS} -lk4a -lpthread

kinect: libfreenect2 libfreenect2/examples/Protonect.cpp
	make -C libfreenect2/build/
	cp libfreenect2/build/bin/Protonect ./kinect

libfreenect2: libfreenect2/CMakeLists.txt
	git submodule update --init
	cd libfreenect2 && mkdir -p build
	cd libfreenect2/build && cmake .. -DENABLE_CXX11=1
	touch libfreenect2/

clean:
	-rm *.o ${TARGETS}
