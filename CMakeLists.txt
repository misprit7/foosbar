cmake_minimum_required(VERSION 2.8)
project( foosball )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )


add_executable( main main.cpp )

target_link_libraries( main ${OpenCV_LIBS} )
target_link_libraries( main /usr/local/lib/libsFoundation20.so )

