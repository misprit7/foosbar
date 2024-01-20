cmake_minimum_required(VERSION 3.5)
project( foosbar )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# opencv
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

# qualisys
find_package( qualisys_cpp_sdk REQUIRED )

# uwebsockets
# find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
# include_directories( /usr/local/include/uWebSockets )

add_executable( foosbar main.cpp algo.cpp )

target_link_libraries( foosbar ${OpenCV_LIBS} )
target_link_libraries( foosbar /usr/local/lib/libsFoundation20.so )
target_link_libraries( foosbar qualisys_cpp_sdk )
target_link_libraries( foosbar ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} usockets )

