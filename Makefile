TARGETS=webcam sur40 realsense kinect k4a
LIBS=gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4

CCFLAGS=-std=c++11 -O3 -Wall -ggdb -I /usr/include/eigen3/ -I include/ $(shell pkg-config --cflags ${LIBS})
LDFLAGS=-std=c++11 -O3 -Wall -ggdb $(shell pkg-config --libs ${LIBS})

all: ${TARGETS}

%.o: %.cpp
	g++ -c -o $@ $< ${CCFLAGS}

webcam_sur40.o: webcam.cpp
	g++ -c -o $@ $< -DSUR40=SUR40 ${CCFLAGS} 

webcam: webcam.o Camera.o V4L2.o
	g++ -o $@ $^ ${LDFLAGS}

sur40: webcam_sur40.o Camera.o SUR40.o
	g++ -o $@ $^ ${LDFLAGS}

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
	-rm *.o ${TARGETS}
