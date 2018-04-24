gstreamer: gstreamer.cpp
	g++ -DGSTREAMER_STANDALONE -Wall -ggdb -o $@ $^ $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv)
