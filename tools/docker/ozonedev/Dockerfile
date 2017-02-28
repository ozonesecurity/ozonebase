FROM ubuntu:16.04
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
    libatlas-base-dev \
    libfontconfig \
    libfreetype6 \
    libfreetype6-dev \
    libjpeg-dev \
    libjpeg8 \
    liblapack-dev \
    libopenblas-dev \
    libtheora-dev \
    libtool \
    libv4l-dev \
    libvorbis-dev \
    pkg-config \
    supervisor \
    texinfo \
    nasm \
    yasm \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
RUN useradd --create-home --shell /bin/bash --password RnXP2VVrWMy/w -G sudo,video ozone
ENV HOME /home/ozone
WORKDIR $HOME
COPY vimrc.txt .vimrc
COPY bootstrap-ozone.sh .
COPY demo/ demo
RUN chown -R ozone:ozone .vimrc bootstrap-ozone.sh demo/
USER ozone
ENV INSTALLDIR "$HOME/install"
ENV PATH "$PATH:$INSTALLDIR/bin"
EXPOSE 9280 9292
