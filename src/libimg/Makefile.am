AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libimg.a

libimg_a_SOURCES = libimgJpeg.c libimgPoly.cpp libimgImage.cpp

pkginclude_HEADERS = \
    libimgJinclude.h \
    libimgJpeg.h \
    libimgRgb.h \
    libimgCoord.h \
    libimgBox.h \
    libimgPoly.h \
    libimgImage.h
