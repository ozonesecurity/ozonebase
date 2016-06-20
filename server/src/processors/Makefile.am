AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libprocessors.a

libprocessors_a_SOURCES = \
	ozDelayBuffer.cpp \
	ozFilterSwapUV.cpp \
	ozImageConvert.cpp \
	ozImageTimestamper.cpp \
	ozMotionDetector.cpp \
	ozMatrixVideo.cpp \
	ozRateLimiter.cpp \
	ozSignalChecker.cpp \
	ozVideoFilter.cpp

pkginclude_HEADERS = \
	ozDelayBuffer.h \
	ozFilterSwapUV.h \
	ozImageConvert.h \
	ozImageTimestamper.h \
	ozMotionDetector.h \
	ozMatrixVideo.h \
	ozRateLimiter.h \
	ozSignalChecker.h \
	ozVideoFilter.h
