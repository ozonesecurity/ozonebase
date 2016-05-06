AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libconsumers.a

libconsumers_a_SOURCES = \
	zmEventRecorder.cpp \
	zmLocalFileDump.cpp \
	zmLocalFileOutput.cpp \
	zmMemoryOutput.cpp \
	zmMovieFileOutput.cpp

pkginclude_HEADERS = \
	zmEventRecorder.h \
	zmLocalFileDump.h \
	zmLocalFileOutput.h \
	zmMemoryOutput.h \
	zmMovieFileOutput.h
