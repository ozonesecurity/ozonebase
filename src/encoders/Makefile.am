AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libencoders.a

libencoders_a_SOURCES = \
	zmH264Encoder.cpp \
	zmJpegEncoder.cpp

pkginclude_HEADERS = \
	zmH264Encoder.h \
	zmJpegEncoder.h
