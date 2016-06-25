# Main CMake file for the Ozonebase project.
# Written by knnniggett (Andrew Bauer)
# Initial draft based on CMake file from the ZoneMinder project, by mastertheknife (Kfir Itzhak)
# For more information and installation, see the INSTALL file
#
cmake_minimum_required (VERSION 2.8)
project (ozonebase)
set(ozonebase_VERSION "2.0.0a")

#
# If we choose to use git submodules, then place a check for the appropraitate submodules here
#

# CMake does not allow out-of-source build if CMakeCache.exists 
# in the source folder. Abort and notify the user
if(
    (NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)) 
    AND (EXISTS "${CMAKE_SOURCE_DIR}/CMakeCache.txt"))
        message(FATAL_ERROR " You are attempting to do an out-of-source build, 
            but a cmake cache file for an in-source build exists. Please delete 
            the file CMakeCache.txt from the source folder to proceed.")
endif(
    (NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)) 
    AND (EXISTS "${CMAKE_SOURCE_DIR}/CMakeCache.txt"))

# Default build type. To change the build type, 
# use the CMAKE_BUILD_TYPE configuration option.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE 
            Release CACHE STRING "Build type: Release or Debug" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Can assist in troubleshooting.
#Package builds will often turn these on automatically during the build process so no need to set these here
#set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_INSTALL_ALWAYS ON)

# Default CLFAGS and CXXFLAGS:
set(CMAKE_C_FLAGS_RELEASE "-Wall -D__STDC_CONSTANT_MACROS -O2")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -D__STDC_CONSTANT_MACROS -O2")
set(CMAKE_C_FLAGS_DEBUG "-Wall -D__STDC_CONSTANT_MACROS -g")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -D__STDC_CONSTANT_MACROS -g")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Modules that we need:
#include (GNUInstallDirs)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
#include(CheckPrototypeDefinition_fixed)
include(CheckTypeSize)
include(CheckStructHasMember)
#include(ac_std_funcs)
include(CheckIncludeFileCXX)
include(TestSignalType)
include(CheckPrototypeExists)
include(TestSTDC)

# Configuration options
#
# Macros commented out are not currently used in the project
#
mark_as_advanced( 
    FORCE 
    OZ_EXTRA_LIBS 
#    OZ_MYSQL_ENGINE 
#    CMAKE_INSTALL_FULL_BINDIR 
#    OZ_TARGET_DISTRO 
    OZ_CONFIG_DIR)

#set(OZ_USER "zoneminder" CACHE STRING "The user apache or the local web server runs on. Leave empty for automatic detection. If that fails, you can use this variable to force")
#set(OZ_WEB_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT}/www" CACHE PATH "Location of the web files, default: <prefix>/${CMAKE_INSTALL_DATADIR}/zoneminder/www")
#set(OZ_CGI_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/${PROJECT}/cgi-bin" CACHE PATH "Location of the cgi-bin files, default: <prefix>/${CMAKE_INSTALL_LIBEXECDIR}/zoneminder/cgi-bin")
#set(OZ_WEB_HOST "localhost" CACHE STRING "Hostname for virtual hosting, default: localhost")
set(OZ_DB_HOST "localhost" CACHE STRING "Hostname where ZoneMinder database located, default: localhost")
set(OZ_DB_NAME "oz" CACHE STRING "Name of ZoneMinder database, default: oz")
set(OZ_DB_USER "ozuser" CACHE STRING "Name of ZoneMinder database user, default: ozuser")
set(OZ_DB_PASS "ozpass" CACHE STRING "Password of ZoneMinder database user, default: ozpass")
#set(OZ_SSL_LIB "gnutls" CACHE STRING "Library to use for ssl functions, default: gnutls")
set(OZ_RUNDIR "/var/run/oz" CACHE PATH "Location of transient process files, default: /var/run/oz")
#set(OZ_TMPDIR "/var/tmp/oz" CACHE PATH "Location of temporary files, default: /tmp/oz")
#set(OZ_LOGDIR "/var/log/oz" CACHE PATH "Location of generated log files, default: /var/log/oz")
#set(OZ_LIBARCH "/lib" CACHE PATH "Architecture specific library path, default lib, try lib64 on 64 bit systems")

#Advanced
#set(OZ_MYSQL_ENGINE "InnoDB" CACHE STRING "MySQL engine to use with database, default: InnoDB")
set(OZ_CONFIG_DIR "/${CMAKE_INSTALL_SYSCONFDIR}" CACHE PATH 
    "Location of ZoneMinder configuration, default system config directory")
set(OZ_EXTRA_LIBS "" CACHE STRING 
    "A list of optional libraries, separated by semicolons, e.g. ssl;theora")


# Required for certain checks to work
set(CMAKE_EXTRA_INCLUDE_FILES 
    ${CMAKE_EXTRA_INCLUDE_FILES} stdio.h stdlib.h math.h signal.h
    )
# Required for including headers from the this folder
include_directories("${CMAKE_BINARY_DIR}")
# This is required to enable searching in lib64 (if exists), do not change
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)

# *** LIBRARY CHECKS ***

# math
find_library(MATH_LIBRARIES m)
if(MATH_LIBRARIES)
	set(HAVE_MATH 1)
	list(APPEND OZ_BIN_LIBS "${MATH_LIBRARIES}")
        set(CMAKE_REQUIRED_INCLUDES "math.h")
        set(CMAKE_REQUIRED_LIBRARIES m)
	check_include_file("math.h" HAVE_MATH_H)
	if(NOT HAVE_MATH_H)
		message(FATAL_ERROR 
                    "This project requires math headers - check that math development packages are installed")
	endif(NOT HAVE_MATH_H)
else(MATH_LIBRARIES)
	message(FATAL_ERROR 
            "This project requires math but it was not found on your system")
endif(MATH_LIBRARIES)

# jpeg
find_package(JPEG)
if(JPEG_FOUND)
	set(HAVE_LIBJPEG 1)
	list(APPEND OZ_BIN_LIBS "${JPEG_LIBRARIES}")
	#link_directories(${JPEG_LIBRARY})
	include_directories("${JPEG_INCLUDE_DIR}")
	set(CMAKE_REQUIRED_INCLUDES "${JPEG_INCLUDE_DIR}")
	check_include_files("stdio.h;jpeglib.h" HAVE_JPEGLIB_H)
	if(NOT HAVE_JPEGLIB_H)
		message(FATAL_ERROR 
                    "ZoneMinder requires libjpeg headers - check that libjpeg development packages are installed")
	endif(NOT HAVE_JPEGLIB_H)
else(JPEG_FOUND)
	message(FATAL_ERROR 
            "ZoneMinder requires jpeg but it was not found on your system")
endif(JPEG_FOUND)

# pthread (using find_library and find_path)
find_library(PTHREAD_LIBRARIES pthread)
if(PTHREAD_LIBRARIES)
	set(HAVE_LIBPTHREAD 1)
	list(APPEND OZ_BIN_LIBS "${PTHREAD_LIBRARIES}")
	find_path(PTHREAD_INCLUDE_DIR pthread.h)
	if(PTHREAD_INCLUDE_DIR)
		include_directories("${PTHREAD_INCLUDE_DIR}")
		set(CMAKE_REQUIRED_INCLUDES "${PTHREAD_INCLUDE_DIR}")
	endif(PTHREAD_INCLUDE_DIR)
	mark_as_advanced(FORCE PTHREAD_LIBRARIES PTHREAD_INCLUDE_DIR)
	check_include_file("pthread.h" HAVE_PTHREAD_H)
	if(NOT HAVE_PTHREAD_H)
		message(FATAL_ERROR 
                    "ZoneMinder requires pthread headers - check that pthread development packages are installed")
	endif(NOT HAVE_PTHREAD_H)
else(PTHREAD_LIBRARIES)
	message(FATAL_ERROR 
            "ZoneMinder requires pthread but it was not found on your system")
endif(PTHREAD_LIBRARIES)

# zlib
find_package(ZLIB)
if(ZLIB_FOUND)
	set(HAVE_LIBZ 1)
	list(APPEND OZ_BIN_LIBS "${ZLIB_LIBRARIES}")
	include_directories("${ZLIB_INCLUDE_DIR}")
	set(CMAKE_REQUIRED_INCLUDES "${ZLIB_INCLUDE_DIR}")
	check_include_file("zlib.h" HAVE_ZLIB_H)
	set(optlibsfound "${optlibsfound} zlib")
else(ZLIB_FOUND)
	set(optlibsnotfound "${optlibsnotfound} zlib")
endif(ZLIB_FOUND)

# avformat (using find_library and find_path)
find_library(AVFORMAT_LIBRARIES avformat)
if(AVFORMAT_LIBRARIES)
    set(HAVE_LIBAVFORMAT 1)
    list(APPEND OZ_BIN_LIBS "${AVFORMAT_LIBRARIES}")
    find_path(AVFORMAT_INCLUDE_DIR "libavformat/avformat.h" /usr/include/ffmpeg)
    if(AVFORMAT_INCLUDE_DIR)
        include_directories("${AVFORMAT_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_INCLUDES "${AVFORMAT_INCLUDE_DIR}")
    endif(AVFORMAT_INCLUDE_DIR)
    mark_as_advanced(FORCE AVFORMAT_LIBRARIES AVFORMAT_INCLUDE_DIR)
    check_include_file("libavformat/avformat.h" HAVE_LIBAVFORMAT_AVFORMAT_H)
else(AVFORMAT_LIBRARIES)
    message(FATAL_ERROR 
            "ZoneMinder requires avformat but it was not found on your system")
endif(AVFORMAT_LIBRARIES)

# avcodec (using find_library and find_path)
find_library(AVCODEC_LIBRARIES avcodec)
if(AVCODEC_LIBRARIES)
    set(HAVE_LIBAVCODEC 1)
    list(APPEND OZ_BIN_LIBS "${AVCODEC_LIBRARIES}")
    find_path(AVCODEC_INCLUDE_DIR "libavcodec/avcodec.h" /usr/include/ffmpeg)
    if(AVCODEC_INCLUDE_DIR)
        include_directories("${AVCODEC_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_INCLUDES "${AVCODEC_INCLUDE_DIR}")
    endif(AVCODEC_INCLUDE_DIR)
    mark_as_advanced(FORCE AVCODEC_LIBRARIES AVCODEC_INCLUDE_DIR)
    check_include_file("libavcodec/avcodec.h" HAVE_LIBAVCODEC_AVCODEC_H)
else(AVCODEC_LIBRARIES)
    message(FATAL_ERROR 
            "ZoneMinder requires avcodec but it was not found on your system")
endif(AVCODEC_LIBRARIES)

# avdevice (using find_library and find_path)
find_library(AVDEVICE_LIBRARIES avdevice)
if(AVDEVICE_LIBRARIES)
    set(HAVE_LIBAVDEVICE 1)
    list(APPEND ZM_BIN_LIBS "${AVDEVICE_LIBRARIES}")
    find_path(AVDEVICE_INCLUDE_DIR "libavdevice/avdevice.h" /usr/include/ffmpeg)
    if(AVDEVICE_INCLUDE_DIR)
        include_directories("${AVDEVICE_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_INCLUDES "${AVDEVICE_INCLUDE_DIR}")
    endif(AVDEVICE_INCLUDE_DIR)
    mark_as_advanced(FORCE AVDEVICE_LIBRARIES AVDEVICE_INCLUDE_DIR)
    check_include_file("libavdevice/avdevice.h" HAVE_LIBAVDEVICE_AVDEVICE_H)
    set(optlibsfound "${optlibsfound} AVDevice")
else(AVDEVICE_LIBRARIES)
    set(optlibsnotfound "${optlibsnotfound} AVDevice")
endif(AVDEVICE_LIBRARIES)

# avutil (using find_library and find_path)
find_library(AVUTIL_LIBRARIES avutil)
if(AVUTIL_LIBRARIES)
    set(HAVE_LIBAVUTIL 1)
    list(APPEND OZ_BIN_LIBS "${AVUTIL_LIBRARIES}")
    find_path(AVUTIL_INCLUDE_DIR "libavutil/avutil.h" /usr/include/ffmpeg)
    if(AVUTIL_INCLUDE_DIR)
        include_directories("${AVUTIL_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_INCLUDES "${AVUTIL_INCLUDE_DIR}")
    endif(AVUTIL_INCLUDE_DIR)
    mark_as_advanced(FORCE AVUTIL_LIBRARIES AVUTIL_INCLUDE_DIR)
    check_include_file("libavutil/avutil.h" HAVE_LIBAVUTIL_AVUTIL_H)
    check_include_file("libavutil/mathematics.h" HAVE_LIBAVUTIL_MATHEMATICS_H)
else(AVUTIL_LIBRARIES)
    message(FATAL_ERROR 
            "ZoneMinder requires avutil but it was not found on your system")
endif(AVUTIL_LIBRARIES)

# swscale (using find_library and find_path)
find_library(SWSCALE_LIBRARIES swscale)
if(SWSCALE_LIBRARIES)
    set(HAVE_LIBSWSCALE 1)
    list(APPEND OZ_BIN_LIBS "${SWSCALE_LIBRARIES}")
    find_path(SWSCALE_INCLUDE_DIR "libswscale/swscale.h" /usr/include/ffmpeg)
    if(SWSCALE_INCLUDE_DIR)
        include_directories("${SWSCALE_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_INCLUDES "${SWSCALE_INCLUDE_DIR}")
    endif(SWSCALE_INCLUDE_DIR)
    mark_as_advanced(FORCE SWSCALE_LIBRARIES SWSCALE_INCLUDE_DIR)
    check_include_file("libswscale/swscale.h" HAVE_LIBSWSCALE_SWSCALE_H)
else(SWSCALE_LIBRARIES)
    message(FATAL_ERROR 
            "ZoneMinder requires swscale but it was not found on your system")
endif(SWSCALE_LIBRARIES)

# libdl
find_library(DL_LIBRARIES dl libdl ltdl libltdl)
if(DL_LIBRARIES)
    set(HAVE_LIBDL 1)
    list(APPEND OZ_BIN_LIBS "${DL_LIBRARIES}")
    find_path(DL_INCLUDE_DIR "dlfcn.h")
    if(DL_INCLUDE_DIR)
        include_directories("${DL_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_INCLUDES "${DL_INCLUDE_DIR}")
    endif(DL_INCLUDE_DIR)
    mark_as_advanced(FORCE DL_LIBRARIES DL_INCLUDE_DIR)
    check_include_file("dlfcn.h" HAVE_DL_H)
else(DL_LIBRARIES)
    message(FATAL_ERROR 
            "This project requires the dynamic link library but it was not found on your system")
endif(DL_LIBRARIES)

# check include files
foreach( INCVAR alloca fcntl inttypes limits memory netdb
         string strings execinfo stdbool stddef stdint syslog
         unistd values
       )
    string( TOUPPER ${INCVAR} INCVAR_U )
    check_include_file("${INCVAR}.h" HAVE_${INCVAR_U}_H)
    if (NOT HAVE_${INCVAR_U}_H)
        message(FATAL_ERROR 
            "${ICVAR} function not found and this project requires it.")
    endif (NOT HAVE_${INCVAR_U}_H)
endforeach(INCVAR)

# System Checks
check_symbol_exists(alloca alloca.h HAVE_ALLOCA)
check_include_file(netinet/in.h HAVE_NETINET_IN_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_symbol_exists(x264_predict_16x16_init x264.h HAVE_LIBX264)
check_include_file_cxx("malloc.h" HAVE_MALLOC_H)
check_include_file("linux/videodev2.h" HAVE_LINUX_VIDEODEV2_H)
check_include_file("sys/mman.h" HAVE_SYS_MMAN_H)
check_include_file("sys/param.h" HAVE_SYS_PARAM_H)
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)

check_function_exists("backtrace" HAVE_DECL_BACKTRACE)
check_function_exists("backtrace_symbols" HAVE_DECL_BACKTRACE_SYMBOLS)
check_function_exists("round" HAVE_DECL_ROUND)

check_type_size("siginfo_t" HAVE_SIGINFO_T)
check_type_size("ucontext_t" HAVE_UCONTEXT_T)
check_type_size("struct sigcontext" HAVE_STRUCT_SIGCONTEXT)
check_struct_has_member("struct sigcontext" eip signal.h HAVE_STRUCT_SIGCONTEXT_EIP)

foreach( FUNCVAR gethostbyname gethostname gettimeofday memmove
         memset select socket sqrt strcasecmp strncasecmp strchr strrchr strcspn strerror
         strsignal strspn strstr strtol strtoull mmap munmap getpagesize mkdir
         putenv strftime malloc
       )
    string( TOUPPER ${FUNCVAR} FUNCVAR_U )
    check_function_exists("${FUNCVAR}" HAVE_${FUNCVAR_U})
    if (NOT HAVE_${FUNCVAR_U})
        message(FATAL_ERROR 
            "${FUNCVAR} function not found and this project requires it.")
    endif (NOT HAVE_${FUNCVAR_U})
endforeach(FUNCVAR)

# Abort if v4l2 headers not found
if (HAVE_LINUX_VIDEODEV2_H)
    set(OZ_V4L 1)
else (HAVE_LINUX_VIDEODEV2_H)
    message(FATAL_ERROR 
        "Video4Linux 2 header files not found and this project requires them.")
endif (HAVE_LINUX_VIDEODEV2_H)

# Abort if mmap not found
if(HAVE_SYS_MMAN_H)
    set(OZ_MEM_MAPPED 1)
endif(HAVE_SYS_MMAN_H)

# Some variables that oz expects
set(OZ_PID "${OZ_RUNDIR}/zm.pid")
#set(BINDIR "${CMAKE_INSTALL_FULL_BINDIR}")
#set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(SYSCONFDIR "/${CMAKE_INSTALL_SYSCONFDIR}")

# Generate files from the .in files
configure_file(src/base/ozConfig.h.in "${CMAKE_CURRENT_BINARY_DIR}/src/base/ozConfig.h" @ONLY)
configure_file(config-cmake.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

# Process subdirectories
add_subdirectory(src/base)
add_subdirectory(src/consumers)
add_subdirectory(src/encoders)
add_subdirectory(src/examples)
add_subdirectory(src/libgen)
add_subdirectory(src/libimg)
add_subdirectory(src/processors)
add_subdirectory(src/protocols)
add_subdirectory(src/providers)

# Print optional libraries detection status
message(STATUS "Optional libraries found:${optlibsfound}")
message(STATUS "Optional libraries not found:${optlibsnotfound}")



