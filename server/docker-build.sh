#!/bin/bash
# Used to prepare to build on docker, needs library and include paths to be set up
aclocal -I m4
autoheader
automake --force-missing --add-missing
autoconf
./configure --prefix=/home/ozone/install LDFLAGS=-L/home/ozone/install/lib CPPFLAGS=-I/home/ozone/install/include
