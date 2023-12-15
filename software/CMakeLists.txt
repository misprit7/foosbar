cmake_minimum_required(VERSION 3.5)
project( foosbar )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package( OpenCV REQUIRED )
find_package(qualisys_cpp_sdk REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )


add_executable( foosbar main.cpp )

target_link_libraries( foosbar ${OpenCV_LIBS} )
target_link_libraries( foosbar /usr/local/lib/libsFoundation20.so )
target_link_libraries( foosbar qualisys_cpp_sdk )

