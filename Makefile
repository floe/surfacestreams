TARGETS=surfacecast
LIBS=gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 glib-2.0 opencv4

CCFLAGS=-std=c++17 -O3 -Wall -ggdb -pg -I /usr/include/eigen3/ -I src/ -I tuio11/ -I tuio11/oscpack/ $(shell pkg-config --cflags ${LIBS}) -MMD -MP
LDFLAGS=-std=c++17 -O3 -Wall -ggdb -pg -lpthread $(shell pkg-config --libs ${LIBS})

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

tuio11/libTUIO.a:
	make -C tuio11 libTUIO.a

%.o: src/%.cpp
	g++ -c -o $@ $< ${CCFLAGS}

-include $(wildcard *.d)

surfacecast: surfacecast.o Camera.o V4L2.o SUR40.o VirtualCam.o ${OBJECTS} tuio11/libTUIO.a
	g++ -o $@ $^ ${LDFLAGS}

clean:
	-rm *.o *.d gmon.out ${TARGETS}

install: loopback ${TARGETS}
	cp -v ${TARGETS} /usr/local/bin/

loopback:
	cp -vn config/v4l2loopback-autoload.conf /etc/modules-load.d/
	cp -vn config/v4l2loopback-options.conf  /etc/modprobe.d/
	modprobe v4l2loopback
