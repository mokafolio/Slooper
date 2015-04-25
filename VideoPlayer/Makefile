CXXFLAGS += -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi
buildDirectory = build
sourceFiles = VideoPlayer.cpp
target = VideoPlayer
objects := $(addprefix $(buildDirectory)/, $(sourceFiles:.cpp=.o))
includes = -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -I./ -I/opt/vc/src/hello_pi/libs/ilclient -I/opt/vc/src/hello_pi/libs/vgfont
libraries += -L/opt/vc/lib/ -L/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/src/hello_pi/libs/vgfont -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -lilclient -lvgfont

#$(info $(libraries))

all: release

release: $(buildDirectory)/$(target)

$(buildDirectory)/$(target): $(objects)
	$(CXX) $(objects) -o $@ $(libraries)

$(buildDirectory)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(includes) -c $< -o $@

clean:
	rm -r -f $(buildDirectory)/*.o
	rm -f $(buildDirectory)/$(target)
