AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libproviders.a

libproviders_a_SOURCES = \
	zmLocalFileInput.cpp \
	zmLocalVideoInput.cpp \
	zmMemoryInput.cpp \
	zmMemoryInputV1.cpp \
	zmNetworkAVInput.cpp \
	zmRawH264Input.cpp \
	zmRemoteVideoInput.cpp \
	zmSlaveVideo.cpp \
	zmVideo4LinuxInput.cpp

pkginclude_HEADERS = \
	zmLocalFileInput.h \
	zmLocalVideoInput.h \
	zmMemoryInput.h \
	zmMemoryInputV1.h \
	zmNetworkAVInput.h \
	zmRawH264Input.h \
	zmRemoteVideoInput.h \
	zmSlaveVideo.h \
	zmVideo4LinuxInput.h
