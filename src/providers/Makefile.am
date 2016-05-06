AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DDB_COMPDEBUG -Wall -Wno-sign-compare

CLEANFILES = *.rpo

lib_LIBRARIES = \
    libproviders.a

libproviders_a_SOURCES = \
	zmLocalFileInput.cpp \
	zmLocalVideoInput.cpp \
	zmMemoryInput.cpp \
	zmNetworkAVInput.cpp \
	zmRemoteVideoInput.cpp \
	zmSlaveVideo.cpp \
	zmVideo4LinuxInput.cpp \
    zmYuanInput.cpp

pkginclude_HEADERS = \
	zmLocalFileInput.h \
	zmLocalVideoInput.h \
	zmMemoryInput.h \
	zmNetworkAVInput.h \
	zmRemoteVideoInput.h \
	zmSlaveVideo.h \
	zmVideo4LinuxInput.h \
    zmYuanInput.h
