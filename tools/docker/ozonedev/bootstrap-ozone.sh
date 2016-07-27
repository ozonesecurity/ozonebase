#!/bin/bash
cd $HOME
mkdir $INSTALLDIR
git clone https://github.com/ozonesecurity/ozonebase.git
cd $HOME/ozonebase
git submodule update --init --recursive
( cd externals/openh264/ && make PREFIX="$INSTALLDIR" install )
( cd externals/ffmpeg && PKG_CONFIG_PATH=$INSTALLDIR/lib/pkgconfig ./configure --enable-shared --enable-libopenh264 --prefix=$INSTALLDIR && make install )
( cd externals/dlib && mkdir build && cd build && cmake .. -DCMAKE_PREFIX_PATH=$INSTALLDIR -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib -DUSE_AVX_INSTRUCTIONS=1 && make install )
( cd server && cmake -DCMAKE_PREFIX_PATH=$INSTALLDIR -DOZ_EXAMPLES=ON -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib . && make install )
