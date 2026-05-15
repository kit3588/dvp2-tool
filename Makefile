CC  = gcc
CXX = g++

# DVP2 libs assumed installed in /usr/lib (from vendor install.sh or package)
LIBS = -L/usr/lib -ldvp -lhzd -lpthread -Wl,-rpath,/usr/lib -std=c++11

TARGET = Demo IPConfigDemo Dvp2StreamCallback ResetCamera CameraInfo

all: $(TARGET)

Demo: Demo.cpp
	$(CXX) -o $@ $< $(LIBS)

IPConfigDemo: IPConfigDemo.cpp
	$(CXX) -o $@ $< $(LIBS)

Dvp2StreamCallback: Dvp2StreamCallback.cpp
	$(CXX) -o $@ $< $(LIBS)

# ResetCamera must force-link libstdc++ so the camera driver .dscam.so plugins
# (loaded lazily via dlopen inside libdvp.so) get a fully-initialized C++
# runtime. Without this, dvpOpenByName segfaults on both GigE and USB cameras.
ResetCamera: ResetCamera.cpp
	$(CXX) -o $@ $< -Wl,--no-as-needed -lstdc++ -Wl,--as-needed $(LIBS)

CameraInfo: CameraInfo.cpp
	$(CXX) -o $@ $< -Wl,--no-as-needed -lstdc++ -Wl,--as-needed $(LIBS)

clean:
	$(RM) -f $(TARGET)
