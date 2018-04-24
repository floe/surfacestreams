gstreamer: gstreamer.cpp
	g++ -DGSTREAMER_STANDALONE -Wall -ggdb -o $@ $^ -O3 -I /usr/include/eigen3/ -I include/ -std=c++11 $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv)

clean:
	rm gstreamer
