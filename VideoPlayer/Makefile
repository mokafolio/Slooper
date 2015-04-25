CXXFLAGS +=
sourceFiles = VideoPlayer.cpp
target = VideoPlayer
objects := $(addprefix build/, $(sourceFiles:.cpp=.o))

all: release

release:
	$(target)

$(target): $(objects)
	$(CXX) $(objects) -o $@

.o:
	$(CXX) $(CXXFLAGS) $< -o $@
