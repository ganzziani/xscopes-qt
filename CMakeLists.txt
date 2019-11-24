# TODO: handle Qt5, win32 and mac
cmake_minimum_required(VERSION 2.8.0)
project(XprotolabInterface)

# debug mode
set(CMAKE_BUILD_TYPE Debug)
# verbose output
set(CMAKE_VERBOSE_MAKEFILE true)

# PIC and c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# target
set(TARGET xscope)

# LIBUSB interface
# PkgConfig seems mandatory
find_package(PkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})

if(UNIX AND APPLE)
link_directories(libs/mac)
elif(WIN32)
link_directories(libs/win)
endif()

#message(status "LIBUSB_INCLUDE_DIRS" ${LIBUSB_INCLUDE_DIRS} "LANGUAGE" ${LANGUAGE})

if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
  add_definitions("-fno-strict-aliasing -Wall")
endif( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )


## System dependencies are found with CMake's conventions
# find_package(Boost REQUIRED COMPONENTS system thread signals)

##############################################################################
# Qt Environment
##############################################################################
# Prevents from clashing with boost
#add_definitions(-DQT_NO_KEYWORDS)

# Add the appropriate components to the component list here
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport SerialPort Network)

#$include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${Qt5_INCLUDE_DIRS}
                    ${Qt5Core_INCLUDE_DIRS}
                    ${Qt5Gui_INCLUDE_DIRS}
                    ${Qt5Widgets_INCLUDE_DIRS}
                    ${Qt5PrintSupport_INCLUDE_DIRS}
                    ${Qt5SerialPort_INCLUDE_DIRS}
                    ${Qt5Network_INCLUDE_DIRS})

set(QT_LIBRARIES ${Qt5_LIBRARIES}
                 ${Qt5Core_LIBRARIES}
                 ${Qt5Gui_LIBRARIES}
                 ${Qt5Widgets_LIBRARIES}
                 ${Qt5PrintSupport_LIBRARIES}
                 ${Qt5SerialPort_LIBRARIES}
                 ${Qt5Network_LIBRARIES})

# Qwt module
#set(QWT_LIBRARIES qwt)

##############################################################################
# Qt Sources
##############################################################################
set(FORMS_SRC xprotolabinterface.ui
    customtheme.ui)
QT5_WRAP_UI(FORMS_HEADERS ${FORMS_SRC})
set(MOC_HEADERS  customtheme.h libusbdevice.h
                qcustomplot.h xprotolabinterface.h)
set(RESOURCES_SRC xprotolabinterface.qrc)
file(GLOB QT_FORMS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${FORMS_SRC})
file(GLOB QT_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${RESOURCES_SRC})
file(GLOB_RECURSE QT_MOC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${MOC_HEADERS})


QT5_ADD_RESOURCES(QT_RESOURCES_CPP ${QT_RESOURCES})
#QT5_WRAP_UI(QT_FORMS_HPP ${QT_FORMS})
# Prevents moc from failing on Parse error at "BOOST_JOIN"
#QT5_WRAP_CPP(QT_MOC_HPP ${QT_MOC}
#             OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED
#            )
##############################################################################
# App Sources
##############################################################################                         )
set(HEADER_SRC
    qcustomplot.h
    libusb.h
    libusbdevice.h
    libusbdeviceinfo.h
    xprotolabinterface.h
    fft.h
    complex.h
    sniffer.h
    customcolors.h
    customtheme.h
    serialportconnection.h
    bootloader.h
    filedownloader.h)
set(CPP_SRC main.cpp
    qcustomplot.cpp
    libusbdevice.cpp
    xprotolabinterface.cpp
    fft.cpp
    complex.cpp
    customcolors.cpp
    customtheme.cpp
    serialportconnection.cpp
    bootloader.cpp
    filedownloader.cpp)

file(GLOB_RECURSE CPP_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS ${CPP_SRC})
#file(GLOB_RECURSE INC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS src/*.hpp)

##############################################################################
# extserialport
##############################################################################

add_subdirectory(extserialport)

##############################################################################
# Binaries
##############################################################################

add_executable(${TARGET}
                ${CPP_SOURCES} ${QT_RESOURCES_CPP} ${QT_FORMS_HPP} ${QT_MOC_HPP}
# display files in the Qt creator's navigator
                ${HEADER_SRC}
                ${FORMS_HEADERS}
              )

target_link_libraries(${TARGET}
                ${LIBUSB_LIBRARIES}
                ${QT_LIBRARIES}
                extserialport
#                ${Boost_LIBRARIES}
#				${QWT_LIBRARIES}
)
# ${catkin_LIBRARIES} ${eigen_LIBRARIES} )

# install
if("${CMAKE_SYSTEM}" MATCHES "Linux")
  install(TARGETS ${TARGET} RUNTIME DESTINATION ${DESTDIR}/usr/bin)
  if(EXISTS "/lib/udev/rules.d/")
    install(FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/extra/linux/64-xscope.rules"
      DESTINATION
      "/lib/udev/rules.d/"
      COMPONENT
      Runtime)
  endif()
else()
  set(DESTDIR ${PROJECT_SOURCE_DIR}/install)
  install(TARGETS ${TARGET} RUNTIME DESTINATION ${DESTDIR}/bin)
  #install(TARGETS ${TARGET} RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
endif()


# data files
#install(DIRECTORY data/
#  DESTINATION ${DESTDIR}/data
#  FILES_MATCHING PATTERN "*.png"
#  PATTERN ".git" EXCLUDE
#)
#install(DIRECTORY octave/
#  DESTINATION ${DESTDIR}/octave
#  FILES_MATCHING PATTERN "*.m"
#)
