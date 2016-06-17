AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libconsumers.a

libconsumers_a_SOURCES = \
	ozEventRecorder.cpp \
	ozLocalFileDump.cpp \
	ozLocalFileOutput.cpp \
	ozMemoryOutput.cpp \
	ozMovieFileOutput.cpp \
	ozMp4FileOutput.cpp

pkginclude_HEADERS = \
	ozEventRecorder.h \
	ozLocalFileDump.h \
	ozLocalFileOutput.h \
	ozMemoryOutput.h \
	ozMovieFileOutput.h \
	ozMp4FileOutput.h
