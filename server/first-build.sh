#!/bin/sh
# first build steps
aclocal
autoheader
automake --force-missing --add-missing
autoconf
./configure 

