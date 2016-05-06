AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libprocessors.a

libprocessors_a_SOURCES = \
	zmImageTimestamper.cpp \
	zmMotionDetector.cpp \
	zmQuadVideo.cpp \
	zmRateLimiter.cpp \
	zmSignalChecker.cpp \
	zmVideoFilter.cpp

pkginclude_HEADERS = \
	zmImageTimestamper.h \
	zmMotionDetector.h \
	zmQuadVideo.h \
	zmRateLimiter.h \
	zmSignalChecker.h \
	zmVideoFilter.h
