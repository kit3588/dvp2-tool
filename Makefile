CC  = gcc
CXX = g++

BINDIR = bin

# DVP2 libs assumed installed in /usr/lib (from vendor install.sh or package)
LIBS = -L/usr/lib -ldvp -lhzd -lpthread -Wl,-rpath,/usr/lib -std=c++11

TARGET = Demo IPConfigDemo Dvp2StreamCallback ResetCamera CameraInfo StrobeDemo gige-ip

all: $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

Demo: Demo.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< $(LIBS)

IPConfigDemo: IPConfigDemo.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< $(LIBS)

Dvp2StreamCallback: Dvp2StreamCallback.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< $(LIBS)

# ResetCamera must force-link libstdc++ so the camera driver .dscam.so plugins
# (loaded lazily via dlopen inside libdvp.so) get a fully-initialized C++
# runtime. Without this, dvpOpenByName segfaults on both GigE and USB cameras.
ResetCamera: ResetCamera.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< -Wl,--no-as-needed -lstdc++ -Wl,--as-needed $(LIBS)

CameraInfo: CameraInfo.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< -Wl,--no-as-needed -lstdc++ -Wl,--as-needed $(LIBS)

StrobeDemo: StrobeDemo.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< -Wl,--no-as-needed -lstdc++ -Wl,--as-needed $(LIBS)

gige-ip: gige-ip.cpp | $(BINDIR)
	$(CXX) -o $(BINDIR)/$@ $< -Wl,--no-as-needed -lstdc++ -Wl,--as-needed $(LIBS)

clean:
	$(RM) -rf $(BINDIR)
