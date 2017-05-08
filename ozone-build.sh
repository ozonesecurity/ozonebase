#!/bin/bash
if [ -z "$INSTALLDIR" ];
then
    echo "Please set variable \$INSTALLDIR to the base path where oZone is to be installed";
    exit;
fi

OZBUILDMODE="Release"
DLIBBUILDMODE="Release"
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${INSTALLDIR}/lib/

if [ "$1" == 'help' ] || [ "$1" == 'h' ]; then
    echo "usage: $0 [release|debug|alldebug]"
    echo ""
    echo "release - no debugging symbols/optimizations enabled"
    echo "debug - ozone libs will be built with debugging symbols, dlib will be built in release mode"
    echo "alldebug - both ozone and dlib will be built in debug mode. Note Dlib can get very slow"
    echo ""
    exit
fi

os=`uname`
if [ "$os" == "Darwin" ];
then
        echo "------------- OSX dirty hack warning ------------------"
        echo "There are several steps for a correct OSX compilation and its not a very clean/tested process today"
        echo "1. Make sure you have brew installed and required libs"
        echo "2. Make sure you modify OPENSSL_ROOT_DIR to the right path where OpenSSL is installed"
        echo "3. Pray your OSX environment is the same as mine"
        echo "-------------------------------------------------------"
fi


if [ "$1" ==  "debug" ];
then
    DLIBBUILDMODE="Release"
    OZBUILDMODE="Debug"
fi

if [ "$1" ==  "alldebug" ];
then
    DLIBBUILDMODE="Debug"
    OZBUILDMODE="Debug"
fi

echo "dlib build mode is: ${DLIBBUILDMODE}"
echo "Ozone build mode is: ${OZBUILDMODE}"

# Disable V4L if on mac
# Also, on mac, we need gcc/g++ not clang.
echo "uname reports Operating System is: ${os}"
if [ "$os" != "Darwin" ];
then
        usev4l="--enable-libv4l2"
        forcecompiler=""
else # hacky stuff for OSX
        export OPENSSL_ROOT_DIR=/usr/local/opt/openssl/
        usev4l=""
        forcecompiler="-DCMAKE_C_COMPILER=/usr/local/bin/gcc-6  -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-6"
        echo "We need to force GCC/+6 in OSX for oZone. setting compiler settings to ${forcecompiler}"
fi

mkdir -p $INSTALLDIR

git submodule update --init --recursive
echo "==================== Building OPENH264 ====================="
( cd externals/openh264/ && make PREFIX="$INSTALLDIR" install )
echo "==================== Building FFMPEG ====================="
( cd externals/ffmpeg && PKG_CONFIG_PATH=`pkg-config --variable pc_path pkg-config`:$INSTALLDIR/lib/pkgconfig ./configure --enable-shared --enable-libopenh264 --enable-libfreetype --enable-libfontconfig ${usev4l}  --prefix=$INSTALLDIR && make install )
echo "==================== Building DLIB ====================="
( cd externals/dlib && mkdir -p build && cd build && cmake .. ${forcecompiler} -DCMAKE_PREFIX_PATH=$INSTALLDIR -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib -DUSE_AVX_INSTRUCTIONS=ON -DCMAKE_VERBOSE_MAKEFILE=ON && cmake --build . --config ${DLIBBUILDMODE}  && make install)
echo "==================== Building JSON ===================="
( cmake ${forcecompiler} -DCMAKE_INCLUDE_PATH=$INSTALLDIR/include -DCMAKE_PREFIX_PATH=$INSTALLDIR  -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib  . && make install )
echo "==================== Building OZONE ====================="
( cd server && cmake ${forcecompiler} -DCMAKE_INCLUDE_PATH=$INSTALLDIR/include -DCMAKE_PREFIX_PATH=$INSTALLDIR -DOZ_EXAMPLES=ON -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib -DCMAKE_BUILD_TYPE=${OZBUILDMODE} . && make install )
