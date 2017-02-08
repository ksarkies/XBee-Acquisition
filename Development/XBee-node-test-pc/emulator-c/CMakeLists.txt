cmake_minimum_required(VERSION 2.8.9)
project (xbee-node-test)
set(SOURCES xbee-node-test.c ../xbee-libs.c ../mainprog-firmware.c serial-libs.c)
add_executable(xbee-node-test ${SOURCES})
set(CMAKE_BUILD_TYPE Debug)

