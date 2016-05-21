AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libencoders.a

libencoders_a_SOURCES = \
	zmH264Encoder.cpp \
	zmH264Relay.cpp \
	zmMpegEncoder.cpp \
	zmJpegEncoder.cpp

pkginclude_HEADERS = \
	zmH264Encoder.h \
	zmH264Relay.h \
	zmMpegEncoder.h \
	zmJpegEncoder.h
