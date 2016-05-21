AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libgen.a

libgen_a_SOURCES = libgenBuffer.cpp libgenDebug.cpp libgenComms.cpp libgenRegexp.cpp libgenThread.cpp libgenTimer.cpp libgenTime.cpp libgenUtils.cpp libgenExec.cpp

EXTRA_DIST = \
    libgenThreadData.cpp

pkginclude_HEADERS = \
    libgenBuffer.h \
    libgenComms.h \
    libgenDebug.h \
    libgenException.h \
    libgenExec.h \
    libgenRegexp.h \
    libgenThread.h \
    libgenTimer.h \
	libgenTime.h \
    libgenUtils.h
