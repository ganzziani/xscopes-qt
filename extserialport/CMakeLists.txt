cmake_minimum_required(VERSION 2.8.0)
project(extserialport)

# todo fix for win
set (EXTSERIALPORT_SRCS
    qextserialport.cpp
    qextserialport_unix.cpp
    qextserialenumerator.cpp
    qextserialenumerator_unix.cpp
)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

add_library(${PROJECT_NAME} SHARED ${EXTSERIALPORT_SRCS})
