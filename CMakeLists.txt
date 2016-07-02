cmake_minimum_required(VERSION 2.8.9)
project (oz_superbuild)

include(ExternalProject)
include(GNUInstallDirs)

# Make sure the submodules are there
if( NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/ffmpeg/configure" )
message(FATAL_ERROR 
        "The git submodules are not available. Please run:
         git submodule update --init --recursive"
       )
endif( NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/ffmpeg/configure" )

# Delete the buildroot staging folder if it exists from a previous build
if( EXISTS "${CMAKE_BINARY_DIR}/buildroot" )
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/buildroot")
endif( EXISTS "${CMAKE_BINARY_DIR}/buildroot" )

ExternalProject_Add(ffmpeg
#                    GIT_REPOSITORY "https://git.ffmpeg.org/ffmpeg.git"
#                    GIT_TAG release/2.8
                    DOWNLOAD_COMMAND ""
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/externals/ffmpeg
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure 
                                      --enable-shared
                                      --libdir=${CMAKE_BINARY_DIR}/buildroot${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
                                      --prefix=${CMAKE_BINARY_DIR}/buildroot${CMAKE_INSTALL_PREFIX}
                    BUILD_COMMAND make
                    INSTALL_COMMAND make install
                   )

# Retrive the ffmpeg source folder
ExternalProject_Get_Property(ffmpeg source_dir)
set(FFMPEG_SOURCE_DIR ${source_dir})
#message(STATUS "FFMPEG_SOURCE_DIR set to: ${FFMPEG_SOURCE_DIR}")

# Retrive the ffmpeg binary folder
ExternalProject_Get_Property(ffmpeg binary_dir)
set(FFMPEG_BINARY_DIR ${binary_dir})
#message(STATUS "FFMPEG_BINARY_DIR set to: ${FFMPEG_BINARY_DIR}")

ExternalProject_Add(ozonebase
                    DOWNLOAD_COMMAND ""
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}/server
                    UPDATE_COMMAND ""
                    CMAKE_ARGS
                              -DOZ_SUPERBUILD=ON
                              -DFFMPEG_SOURCE_DIR=${FFMPEG_SOURCE_DIR}
                              -DFFMPEG_BINARY_DIR=${FFMPEG_BINARY_DIR}
                              -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/buildroot/${CMAKE_INSTALL_PREFIX}
                              -DOZ_EXAMPLES=${OZ_EXAMPLES}
                   )

add_dependencies(ozonebase ffmpeg)

message(STATUS
        "NOTICE: This build has been configured for a cmake superbuild.

        The make command will build each project into the following staging folder: 
        ${CMAKE_BINARY_DIR}/buildroot
        "
       )

install(DIRECTORY ${CMAKE_BINARY_DIR}/buildroot/${CMAKE_INSTALL_PREFIX}/
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        USE_SOURCE_PERMISSIONS
       )

