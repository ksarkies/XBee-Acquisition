cmake_minimum_required(VERSION 2.8.12.2)
project (xbee-node-test)
set(SOURCES xbee-node-test.c ../xbee-libs.c ../mainprog-firmware.c serial-libs.c)
add_executable(xbee-node-test ${SOURCES})

