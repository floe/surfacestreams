TARGETS=webcam sur40 realsense

all: $(TARGETS) kinect

%.o: %.cpp
	g++ -c -D$(basename $@) -std=c++11 -O3 -Wall -ggdb -o $@ $^ -I /usr/include/eigen3/ -I include/ $(shell pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv)

$(TARGETS): %: %.o common.o
	g++ -std=c++11 -O3 -Wall -ggdb -o $@ $^ $(shell pkg-config --libs gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv) -lrealsense2

sur40.o: webcam.o
	cp $^ $@

kinect: libfreenect2 libfreenect2/examples/Protonect.cpp
	make -C libfreenect2/build/
	cp libfreenect2/build/bin/Protonect ./kinect

libfreenect2: libfreenect2/CMakeLists.txt
	git submodule update --init
	cd libfreenect2 && mkdir -p build
	cd libfreenect2/build && cmake .. -DENABLE_CXX11=1
	touch libfreenect2/

clean:
	-rm $(TARGETS) *.o
