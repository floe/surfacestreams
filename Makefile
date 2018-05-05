webcam: webcam.cpp common.cpp
	g++ -Wall -ggdb -o $@ $^ -O3 -I /usr/include/eigen3/ -I include/ -std=c++11 $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv)

#kinect:
#	git submodule update --init
#	cd libfreenect2; mkdir -p build; cd build
#	cmake .. -DENABLE_CXX11=1
#	make

clean:
	rm webcam
