FROM ubuntu:14.04
MAINTAINER Philip Coombes <phil@web2wire.com>
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y \
    nodejs-legacy \
    npm \
    build-essential \
    make \
    cmake \
    git \
    vim \
    libass-dev \
    libfreetype6-dev \
    libjpeg-dev \
    libjpeg8 \
    libtheora-dev \
    libtool \
    libvorbis-dev \
    pkg-config \
    texinfo \
    nasm \
    yasm \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
RUN useradd --create-home --shell /bin/bash --password RnXP2VVrWMy/w -G sudo ozone
ENV HOME /home/ozone
WORKDIR $HOME
COPY vimrc.txt .vimrc
ADD demo.tgz .
RUN chown -R ozone:ozone .vimrc demo/
USER ozone
ENV INSTALLDIR "$HOME/install"
RUN mkdir $INSTALLDIR && git clone https://github.com/ozonesecurity/ozonebase.git ozonebase # Head commit is def7ff9
WORKDIR $HOME/ozonebase
RUN git submodule update --init --recursive && \ 
    ( cd externals/openh264/ && make PREFIX="$INSTALLDIR" install && make clean ) && \
    ( cd externals/ffmpeg && PKG_CONFIG_PATH=$INSTALLDIR/lib/pkgconfig ./configure --enable-shared --enable-libopenh264 --prefix=$INSTALLDIR && make install && make clean ) && \
    ( cd server && cmake -DCMAKE_PREFIX_PATH=$INSTALLDIR -DOZ_EXAMPLES=ON -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DCMAKE_INSTALL_RPATH=$INSTALLDIR/lib . && make install && make clean )
##WORKDIR $HOME/demo
#RUN npm install
#ENV PATH $PATH:$HOME/demo/node_modules/.bin
WORKDIR $HOME
EXPOSE 9280 9292
