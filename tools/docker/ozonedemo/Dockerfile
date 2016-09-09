FROM web2wire/ozonedev:latest
MAINTAINER Philip Coombes <phil@web2wire.com>
ENV DEBIAN_FRONTEND noninteractive
USER ozone
WORKDIR $HOME
RUN ./bootstrap-ozone.sh
RUN rm -rf ozonebase
WORKDIR $HOME/demo
COPY shape_predictor_68_face_landmarks.dat face-input.mp4 ./
RUN npm install
ENV PATH $PATH:$HOME/demo/node_modules/.bin
USER root
RUN apt-get purge -y \
    build-essential \
    cmake \
    make \
    libass-dev \
    libfreetype6-dev \
    libjpeg-dev \
    libtheora-dev \
    libtool \
    libvorbis-dev \
    pkg-config \
    texinfo \
    nasm \
    yasm \
    zlib1g-dev \
    && apt-get clean autoclean \
    && apt-get autoremove -y \
    && rm -rf /var/lib/{apt,dpkg,cache,log}
USER ozone
EXPOSE 9280 9292
CMD [ "/bin/bash", "/home/ozone/demo/run-demo.sh" ]
