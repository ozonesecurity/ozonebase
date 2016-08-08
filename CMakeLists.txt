cmake_minimum_required(VERSION 2.8.9)
project (oz_superbuild)

include(ExternalProject)
include(GNUInstallDirs)

# Make sure the submodules are there
if( NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/ffmpeg/configure" OR
    NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/openh264/Makefile" )
message(FATAL_ERROR 
        "The git submodules are not available. Please run:
         git submodule update --init --recursive"
       )
endif( NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/ffmpeg/configure" OR
    NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/openh264/Makefile" )

# Default build type. To change the build type, 
# use the CMAKE_BUILD_TYPE configuration option.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE 
            Release CACHE STRING "Build type: Release or Debug" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(OZ_EXAMPLES "OFF" CACHE BOOL "Set to ON to build the oZonebase example binaries. Default: OFF")
set(OZ_KEEP_BUILDROOT "OFF" CACHE BOOL "Set to ON to prevent oZonebase from deleting the buildroot folder from a previous build attempt. Default: OFF")

# Internal variables. Not to be modified by the end user
set(OZ_SUPERBUILD "ON" CACHE BOOL INTERNAL)
set(FFMPEG_PREFIX "${CMAKE_BINARY_DIR}/buildroot${CMAKE_INSTALL_PREFIX}" CACHE PATH INTERNAL)
set(FFMPEG_LIBDIR "${FFMPEG_PREFIX}/${CMAKE_INSTALL_LIBDIR}" CACHE PATH INTERNAL)
set(FFMPEG_INCLUDEDIR "${FFMPEG_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}" CACHE PATH INTERNAL)

# Delete the buildroot staging folder if it exists from a previous build
if( NOT ${OZ_KEEP_BUILDROOT} AND EXISTS "${CMAKE_BINARY_DIR}/buildroot" )
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/buildroot")
endif( NOT ${OZ_KEEP_BUILDROOT} AND EXISTS "${CMAKE_BINARY_DIR}/buildroot" )

ExternalProject_Add(dlib
#                    GIT_REPOSITORY "https://github.com/davisking/dlib"
#                    GIT_TAG master
                    DOWNLOAD_COMMAND ""
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/externals/dlib
                    UPDATE_COMMAND ""
                    CMAKE_ARGS
                              -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/buildroot${CMAKE_INSTALL_PREFIX}
                              -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                              -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
                              -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                              -DLIB_INSTALL_DIR=${CMAKE_INSTALL_LIBDIR}
                   )


ExternalProject_Add(openh264
#                    GIT_REPOSITORY "https://github.com/cisco/openh264"
#                    GIT_TAG master
                    DOWNLOAD_COMMAND ""
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/externals/openh264
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND make
                    INSTALL_COMMAND make install
                                    DESTDIR=${CMAKE_BINARY_DIR}/buildroot
                                    PREFIX=${CMAKE_INSTALL_PREFIX}
                                    LIBDIR_NAME=${CMAKE_INSTALL_LIBDIR}
                                    SHAREDLIB_DIR=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
                    BUILD_IN_SOURCE 1
                   )

ExternalProject_Add(ffmpeg
#                    GIT_REPOSITORY "https://git.ffmpeg.org/ffmpeg.git"
#                    GIT_TAG release/2.8
                    DOWNLOAD_COMMAND ""
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/externals/ffmpeg
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND PKG_CONFIG_PATH=${FFMPEG_LIBDIR}/pkgconfig <SOURCE_DIR>/configure 
                                      --enable-shared
                                      --libdir=${FFMPEG_LIBDIR}
                                      --prefix=${FFMPEG_PREFIX}
                                      --enable-libopenh264
                                      --extra-ldflags=-L${FFMPEG_LIBDIR}
                                      --extra-cflags=-I${FFMPEG_INCLUDEDIR}/wels
                                      --pkg-config-flags=--define-variable=libdir=${FFMPEG_LIBDIR}
                                      --pkg-config-flags=--define-variable=includedir=${FFMPEG_INCLUDEDIR}
                    BUILD_COMMAND make
                    INSTALL_COMMAND make install && make install-libs
                   )

add_dependencies(ffmpeg openh264)

ExternalProject_Add(ozonebase
                    DOWNLOAD_COMMAND ""
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/server
                    UPDATE_COMMAND ""
                    CMAKE_ARGS
                              -DOZ_SUPERBUILD=${OZ_SUPERBUILD}
                              -DFFMPEG_LIBDIR=${FFMPEG_LIBDIR}
                              -DFFMPEG_INCLUDEDIR=${FFMPEG_INCLUDEDIR}
                              -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/buildroot${CMAKE_INSTALL_PREFIX}
                              -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                              -DOZ_EXAMPLES=${OZ_EXAMPLES}
                              -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
                              -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   )

add_dependencies(ozonebase ffmpeg)

message(STATUS
        "NOTICE:This build has been configured for a cmake superbuild.\nThe make command will build each project into the following staging folder:\n${CMAKE_BINARY_DIR}/buildroot"
       )

install(DIRECTORY ${CMAKE_BINARY_DIR}/buildroot/${CMAKE_INSTALL_PREFIX}/
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        USE_SOURCE_PERMISSIONS
       )

