cmake_minimum_required (VERSION 2.8)
project (GVideoPlayer)

include_directories(/usr/include/gstreamer-1.0 /usr/include/glib-2.0 /usr/lib/arm-linux-gnueabihf/glib-2.0/include)

add_executable(GVideoPlayer GVideoPlayer.cpp)
target_link_libraries(GVideoPlayer gstreamer-1.0 pthread)