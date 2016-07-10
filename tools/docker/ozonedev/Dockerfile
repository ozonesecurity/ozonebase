FROM ubuntu:14.04
MAINTAINER Philip Coombes <phil@web2wire.com>
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y \
    nodejs-legacy \
    npm \
    build-essential \
    make \
    git \
    vim \
    autoconf \
    automake \
    libass-dev \
    libfreetype6-dev \
    libjpeg-dev \
    libjpeg8 \
    libtheora-dev \
    libtool \
    libvorbis-dev \
    pkg-config \
    texinfo \
    yasm \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
RUN useradd --create-home --shell /bin/bash --password RnXP2VVrWMy/w -G sudo ozone
USER ozone
ENV HOME /home/ozone
WORKDIR $HOME
RUN mkdir install && git clone https://github.com/ozonesecurity/ozonebase.git ozonebase # Head commit is abb46fb
WORKDIR $HOME/ozonebase
RUN git submodule init && git submodule update
WORKDIR $HOME/ozonebase/externals/ffmpeg
RUN ./configure --prefix=/home/ozone/install --enable-shared && make && make install
WORKDIR $HOME/ozonebase/server
ENV LD_LIBRARY_PATH /home/ozone/install/lib
RUN ./docker-build.sh && make && make install
WORKDIR $HOME
USER root
COPY vimrc.txt .vimrc
ADD demo.tgz .
RUN chown -R ozone:ozone .vimrc demo/
USER ozone
WORKDIR $HOME/demo
RUN npm install
WORKDIR $HOME
ENV PATH $PATH:$HOME/demo/node_modules/.bin
EXPOSE 9280 9292
