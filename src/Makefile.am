AUTOMAKE_OPTIONS = gnu

AM_CPPFLAGS = @MYSQL_CFLAGS@ @FFMPEG_CFLAGS@ -Wall -Wno-sign-compare -fno-inline
#AM_CXXFLAGS = -frepo

CLEANFILES = *.rpo

SUBDIRS = \
	libgen \
	libimg \
	protocols \
	encoders \
	processors \
	consumers \
	providers
		 
bin_PROGRAMS = \
	zmx

zmx_SOURCES = \
    zmx.cpp \
    zmApp.cpp \
    zmFeedBase.cpp \
    zmFeedFrame.cpp \
    zmFeedProvider.cpp \
    zmFeedConsumer.cpp \
    zmMotionData.cpp \
    zmAlarmFrame.cpp \
    zmMotionFrame.cpp \
    zmMemoryIO.cpp \
    zmMemoryIOV1.cpp \
    zmConfig.cpp \
    zmDb.cpp \
    zmRegion.cpp \
    zmZone.cpp \
    zmEncoder.cpp \
    zmStream.cpp \
    zmListener.cpp \
    zmSignal.cpp \
    zmController.cpp \
    zmConnection.cpp \
    zmFfmpeg.cpp

zmx_DEPENDENCIES = */*.a
zmx_LDADD = \
	-L./protocols -lprotocols \
	-L./encoders -lencoders \
	-L./processors -lprocessors \
	-L./consumers -lconsumers \
	-L./providers -lproviders \
	-L./libimg -limg \
	-L./libgen -lgen

#imgconv_SOURCES = imgconv.cpp 
#imgconv_LDADD = -L./libimg -limg -L./libgen -lgen

noinst_HEADERS = 

EXTRA_DIST = \
	zmConfig.h.in

dist-hook:
	@( rm $(distdir)/zmConfig.h )
