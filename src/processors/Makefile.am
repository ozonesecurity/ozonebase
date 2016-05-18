AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libprocessors.a

libprocessors_a_SOURCES = \
	zmDelayBuffer.cpp \
	zmFilterSwapUV.cpp \
	zmImageConvert.cpp \
	zmImageTimestamper.cpp \
	zmMotionDetector.cpp \
	zmQuadVideo.cpp \
	zmRateLimiter.cpp \
	zmSignalChecker.cpp \
	zmVideoFilter.cpp

pkginclude_HEADERS = \
	zmDelayBuffer.h \
	zmFilterSwapUV.h \
	zmImageConvert.h \
	zmImageTimestamper.h \
	zmMotionDetector.h \
	zmQuadVideo.h \
	zmRateLimiter.h \
	zmSignalChecker.h \
	zmVideoFilter.h
