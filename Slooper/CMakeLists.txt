cmake_minimum_required (VERSION 2.8)
project (Slooper)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

include_directories("..")

set(SlooperSources
	Networking/Socket.cpp
	Networking/TCPAcceptor.cpp
	Networking/TCPSocket.cpp)

add_library(Slooper STATIC ${SlooperSources})
add_executable(TestServer TestServer.cpp)
target_link_libraries(TestServer Slooper)
add_executable(TestClient TestClient.cpp)
target_link_libraries(TestClient Slooper)
