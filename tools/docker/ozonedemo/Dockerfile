FROM web2wire/ozonedev:v1
MAINTAINER Philip Coombes <phil@web2wire.com>
ENV DEBIAN_FRONTEND noninteractive
USER root
RUN apt-get purge -y \
    build-essential \
    make \
    autoconf \
    automake \
    libass-dev \
    libfreetype6-dev \
    libjpeg-dev \
    libtheora-dev \
    libtool \
    libvorbis-dev \
    pkg-config \
    texinfo \
    yasm \
    zlib1g-dev \
    && apt-get clean autoclean \
    && apt-get autoremove -y \
    && rm -rf /var/lib/{apt,dpkg,cache,log}
USER ozone
WORKDIR $HOME
RUN rm -rf ozonebase
EXPOSE 9280 9292
CMD [ "/bin/bash", "/home/ozone/demo/run-demo.sh" ]
