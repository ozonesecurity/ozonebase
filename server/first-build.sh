#!/bin/sh
# first build steps
# in ubuntu, sudo apt-get automake libssl-dev
aclocal
autoheader
automake --force-missing --add-missing
autoconf
./configure 

