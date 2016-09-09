#!/bin/bash
cd $HOME
mkdir -p $INSTALLDIR
if [ ! -d "ozonebase" ]
then
  git clone https://github.com/ozonesecurity/ozonebase.git
fi
cd $HOME/ozonebase
git submodule update --init --recursive
( cd externals/openh264/ && make PREFIX="$INSTALLDIR" install )
( cd externals/ffmpeg && PKG_CONFIG_PATH=$INSTALLDIR/lib/pkgconfig ./configure --enable-shared --enable-libv4l2 --enable-libopenh264 --enable-libfreetype --enable-libfontconfig --prefix=$INSTALLDIR && make install )
( cd externals/dlib && mkdir -p build && cd build && cmake .. -DCMAKE_PREFIX_PATH=$INSTALLDIR -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib -DUSE_AVX_INSTRUCTIONS=ON -DCMAKE_VERBOSE_MAKEFILE=ON && cmake --build . --config Release && make install )
( cd server && cmake -DCMAKE_PREFIX_PATH=$INSTALLDIR -DOZ_EXAMPLES=ON -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib -DCMAKE_BUILD_TYPE=Release . && make install )
