TARGETS=surfacecast
LIBS=gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 glib-2.0 opencv4

CCFLAGS=-std=c++11 -O3 -Wall -ggdb -pg -I /usr/include/eigen3/ -I include/ $(shell pkg-config --cflags ${LIBS})
LDFLAGS=-std=c++11 -O3 -Wall -ggdb -pg -lpthread $(shell pkg-config --libs ${LIBS})

# check for K4A include files
ifneq ($(wildcard /usr/include/k4a/),)
	CCFLAGS+=-DK4A
	LDFLAGS+=-lk4a
	OBJECTS+=KinectAzure.o
endif

# check for Realsense include files
ifneq ($(wildcard /usr/include/librealsense2/),)
	CCFLAGS+=-DREALSENSE
	LDFLAGS+=-lrealsense2
	OBJECTS+=Realsense.o
endif

all: ${TARGETS}

%.o: %.cpp
	g++ -c -o $@ $< ${CCFLAGS}

surfacecast: surfacecast.o Camera.o V4L2.o SUR40.o VirtualCam.o ${OBJECTS}
	g++ -o $@ $^ ${LDFLAGS}

clean:
	-rm *.o ${TARGETS}
