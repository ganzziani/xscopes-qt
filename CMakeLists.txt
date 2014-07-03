# TODO: handle Qt5, win32 and mac
cmake_minimum_required(VERSION 2.8.0)
project(XprotolabInterface)

# debug mode
set(CMAKE_BUILD_TYPE Debug)
# verbose output
set(CMAKE_VERBOSE_MAKEFILE true)

# PIC and c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11")

# target
set(TARGET xscope)

# LIBUSB interface
# PkgConfig seems mandatory
find_package(PkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})

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
find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

# Qwt module
#set(QWT_LIBRARIES qwt)

##############################################################################
# Qt Sources
##############################################################################
set(FORMS_SRC xprotolabinterface.ui
    customtheme.ui)
set(MOC_HEADERS  customtheme.h libusbdevice.h
                qcustomplot.h xprotolabinterface.h)
set(RESOURCES_SRC xprotolabinterface.qrc)
file(GLOB QT_FORMS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${FORMS_SRC})
file(GLOB QT_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${RESOURCES_SRC})
file(GLOB_RECURSE QT_MOC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${MOC_HEADERS})

QT4_ADD_RESOURCES(QT_RESOURCES_CPP ${QT_RESOURCES})
QT4_WRAP_UI(QT_FORMS_HPP ${QT_FORMS})
# Prevents moc from failing on Parse error at "BOOST_JOIN"
QT4_WRAP_CPP(QT_MOC_HPP ${QT_MOC}
#             OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED
            )
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
    customtheme.h)
set(CPP_SRC main.cpp
    qcustomplot.cpp
    libusbdevice.cpp
    xprotolabinterface.cpp
    fft.cpp
    complex.cpp
    customtheme.cpp)

file(GLOB_RECURSE CPP_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS ${CPP_SRC})
#file(GLOB_RECURSE INC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS src/*.hpp)

##############################################################################
# Binaries
##############################################################################

add_executable(${TARGET}
                ${CPP_SOURCES} ${QT_RESOURCES_CPP} ${QT_FORMS_HPP} ${QT_MOC_HPP}
# display files in the Qt creator's navigator
                ${HEADER_SRC}
              )

target_link_libraries(${TARGET} ${QT_LIBRARIES}
                ${LIBUSB_LIBRARIES}
#                ${Boost_LIBRARIES}
#				${QWT_LIBRARIES}
)
# ${catkin_LIBRARIES} ${eigen_LIBRARIES} )

# install
set(DESTDIR ${PROJECT_SOURCE_DIR}/install)
install(TARGETS ${TARGET} RUNTIME DESTINATION ${DESTDIR}/bin)
#install(TARGETS ${TARGET} RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

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
