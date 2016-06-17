AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libproviders.a

libproviders_a_SOURCES = \
	ozLocalFileInput.cpp \
	ozLocalVideoInput.cpp \
	ozMemoryInput.cpp \
	ozMemoryInputV1.cpp \
	ozNetworkAVInput.cpp \
	ozRawH264Input.cpp \
	ozRemoteVideoInput.cpp \
	ozSlaveVideo.cpp \
	ozVideo4LinuxInput.cpp

pkginclude_HEADERS = \
	ozLocalFileInput.h \
	ozLocalVideoInput.h \
	ozMemoryInput.h \
	ozMemoryInputV1.h \
	ozNetworkAVInput.h \
	ozRawH264Input.h \
	ozRemoteVideoInput.h \
	ozSlaveVideo.h \
	ozVideo4LinuxInput.h
