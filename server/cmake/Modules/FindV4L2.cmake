# Video4Linux v2 Library include paths and libraries
#
# Based on CMake script for:
# Locate Intel Threading Building Blocks include paths and libraries
# FindTBB.cmake can be found at https://code.google.com/p/findtbb/
# Written by Hannes Hofmann <hannes.hofmann _at_ informatik.uni-erlangen.de>
# Improvements by Gino van den Bergen <gino _at_ dtecta.com>,
#   Florian Uhlig <F.Uhlig _at_ gsi.de>,
#   Jiri Marsik <jiri.marsik89 _at_ gmail.com>

# The MIT License
#
# Copyright (c) 2011 Hannes Hofmann
# Copyright (c) 2012 Patrick Bellasi
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# This module respects
# V4L2_INSTALL_DIR or $ENV{V4L2_INSTALL_DIR} or $ENV{V4L_INSTALL_DIR}

# This module defines
# V4L2_INCLUDE_DIRS, where to find libv4l2.h, etc.
# V4L2_LIBRARY_DIRS, where to find libv4l2, libv4l1 and libv4lconverter
# V4L2_INSTALL_DIR, the base V4L2 install directory
# V4L2_LIBRARIES, the libraries to link against to use Video4Linux2.
# V4L2_FOUND, If false, don't try to use Video4Linux2.

if (UNIX)
    # LINUX
    set(_V4L2_DEFAULT_INSTALL_DIR "/usr/local/include" "/usr/include")
    set(_V4L2_LIB_NAME "v4l2")
    set(_V4L1_LIB_NAME "v4l1")
    set(_V4L_CONVERTER_NAME "v4lconvert")
endif (UNIX)

#-- Clear the public variables
set (V4L2_FOUND "NO")

#-- Find V4L2 install dir and set ${_V4L2_INSTALL_DIR} and cached ${V4L2_INSTALL_DIR}
# first: use CMake variable V4L2_INSTALL_DIR
if (V4L2_INSTALL_DIR)
    set (_V4L2_INSTALL_DIR ${V4L2_INSTALL_DIR})
endif (V4L2_INSTALL_DIR)

# second: use environment variable
if (NOT _V4L2_INSTALL_DIR)
    if (NOT "$ENV{V4L2_INSTALL_DIR}" STREQUAL "")
        set (_V4L2_INSTALL_DIR $ENV{V4L2_INSTALL_DIR})
    endif (NOT "$ENV{V4L2_INSTALL_DIR}" STREQUAL "")
    if (NOT "$ENV{V4L_INSTALL_DIR}" STREQUAL "")
        set (_V4L2_INSTALL_DIR $ENV{V4L_INSTALL_DIR})
    endif (NOT "$ENV{V4L_INSTALL_DIR}" STREQUAL "")
endif (NOT _V4L2_INSTALL_DIR)

# third: try to find path automatically
if (NOT _V4L2_INSTALL_DIR)
    if (_V4L2_DEFAULT_INSTALL_DIR)
        set (_V4L2_INSTALL_DIR ${_V4L2_DEFAULT_INSTALL_DIR})
    endif (_V4L2_DEFAULT_INSTALL_DIR)
endif (NOT _V4L2_INSTALL_DIR)

# sanity check
if (NOT _V4L2_INSTALL_DIR)
    message ("ERROR: Unable to find V4L install directory. ${_V4L2_INSTALL_DIR}")
else (NOT _V4L2_INSTALL_DIR)
    # finally: set the cached CMake variable V4L2_INSTALL_DIR
    if (NOT V4L2_INSTALL_DIR)
        set (V4L2_INSTALL_DIR ${_V4L2_INSTALL_DIR} CACHE PATH "Video4Linux install directory")
        mark_as_advanced(V4L2_INSTALL_DIR)
    endif (NOT V4L2_INSTALL_DIR)


    #-- Look for include directory and set ${V4L2_INCLUDE_DIR}
    set (V4L2_INC_SEARCH_DIR ${_V4L2_INSTALL_DIR}/include)
    find_path(V4L2_INCLUDE_DIR
        libv4l2.h
        PATHS ${V4L2_INC_SEARCH_DIR} ENV CPATH)
    mark_as_advanced(V4L2_INCLUDE_DIR)


    #-- Look for libraries
    set (_V4L2_LIBRARY_DIR ${_V4L2_INSTALL_DIR}/lib)
    find_library(V4L2_LIBRARY ${_V4L2_LIB_NAME} HINTS ${_V4L2_LIBRARY_DIR}
        PATHS ENV LIBRARY_PATH ENV LD_LIBRARY_PATH)
    find_library(V4L1_LIBRARY ${_V4L1_LIB_NAME} HINTS ${_V4L2_LIBRARY_DIR}
        PATHS ENV LIBRARY_PATH ENV LD_LIBRARY_PATH)
    find_library(V4L_CONVERTER_LIBRARY ${_V4L_CONVERTER_NAME} HINTS ${_V4L2_LIBRARY_DIR}
        PATHS ENV LIBRARY_PATH ENV LD_LIBRARY_PATH)

    #Extract path from V4L2_LIBRARY name
    get_filename_component(V4L2_LIBRARY_DIR ${V4L2_LIBRARY} PATH)
    mark_as_advanced(V4L2_LIBRARY V4L1_LIBRARY V4L_CONVERTER_LIBRARY)

    if (V4L2_INCLUDE_DIR)
        if (V4L2_LIBRARY)
            set (V4L2_FOUND "YES")
            set (V4L2_LIBRARIES ${V4L2_LIBRARY} ${V4L1_LIBRARY})
            set (V4L2_INCLUDE_DIRS ${V4L2_INCLUDE_DIR} CACHE PATH "V4L2 include directory" FORCE)
            set (V4L2_LIBRARY_DIRS ${V4L2_LIBRARY_DIR} CACHE PATH "V4L2 library directory" FORCE)
            mark_as_advanced(V4L2_INCLUDE_DIRS V4L2_LIBRARY_DIRS V4L2_LIBRARIES)
            message(STATUS "Found Video4Linux2")
        endif (V4L2_LIBRARY)
    endif (V4L2_INCLUDE_DIR)

    if (NOT V4L2_FOUND)
        message("ERROR: Video4Linux2 NOT found!")
        message(STATUS "Looked for V4L2 in ${_V4L2_INSTALL_DIR}")
        # do only throw fatal, if this pkg is REQUIRED
        if (V4L2_FIND_REQUIRED)
            message(FATAL_ERROR "Could NOT find V4L2 library.")
        endif (V4L2_FIND_REQUIRED)
    endif (NOT V4L2_FOUND)
endif (NOT _V4L2_INSTALL_DIR)