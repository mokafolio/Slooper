cmake_minimum_required (VERSION 2.8)
project (Slooper)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

include_directories("..")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	include_directories("/opt/vc/include/" "/opt/vc/include/interface/vmcs_host/linux" "/opt/vc/src/hello_pi/libs/ilclient" "/opt/vc/src/hello_pi/libs/vgfont/" "/opt/vc/include/interface/vcos/pthreads")
endif()

set(SlooperSources
	Networking/Socket.cpp
	Networking/TCPAcceptor.cpp
	Networking/TCPSocket.cpp)

add_library(Slooper STATIC ${SlooperSources})
add_executable(TestServer TestServer.cpp)
target_link_libraries(TestServer Slooper pthread)
add_executable(TestClient TestClient.cpp)
target_link_libraries(TestClient Slooper pthread)
add_executable(VideoServer VideoServer.cpp)
target_link_libraries(VideoServer Slooper pthread)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	link_directories(/opt/vc/lib /opt/vc/src/hello_pi/libs/vgfont /opt/vc/src/hello_pi/libs/ilclient)
	add_executable(TestVideoPlayer TestVideoPlayer.cpp)
	message("Linking on raspberry pi!")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi")
	target_link_libraries(TestVideoPlayer Slooper GLESv2 EGL openmaxil bcm_host vcos vchiq_arm pthread rt m ilclient vgfont)
endif()
