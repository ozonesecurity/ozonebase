AUTOMAKE_OPTIONS = gnu

AM_CPPFLAGS = @MYSQL_CFLAGS@ @FFMPEG_CFLAGS@ -Wall -Wno-sign-compare -fno-inline
#AM_CXXFLAGS = -frepo

CLEANFILES = *.rpo

SUBDIRS = \
	libgen \
	libimg \
	base \
	protocols \
	encoders \
	processors \
	consumers \
	providers \
    examples
		 
noinst_HEADERS = 
