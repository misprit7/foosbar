cmake_minimum_required(VERSION 3.5)
project( foosbar )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# opencv
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

# qualisys
find_package( qualisys_cpp_sdk REQUIRED )

# uwebsockets
find_package( OpenSSL REQUIRED )
find_package( libuv REQUIRED )
include_directories( ${OPENSSL_INCLUDE_DIR} )
include_directories( /usr/include/uWebSockets )

add_executable( foosbar main.cpp )

target_link_libraries( foosbar ${OpenCV_LIBS} )
target_link_libraries( foosbar /usr/local/lib/libsFoundation20.so )
target_link_libraries( foosbar qualisys_cpp_sdk )
target_link_libraries( foosbar uWS OpenSSL::SSL OpenSSL::Crypto libuv )

