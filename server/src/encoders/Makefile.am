AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libencoders.a

libencoders_a_SOURCES = \
	ozH264Encoder.cpp \
	ozH264Relay.cpp \
	ozMpegEncoder.cpp \
	ozJpegEncoder.cpp

pkginclude_HEADERS = \
	ozH264Encoder.h \
	ozH264Relay.h \
	ozMpegEncoder.h \
	ozJpegEncoder.h
