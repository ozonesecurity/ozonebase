AUTOMAKE_OPTIONS = gnu

AM_CPPFLAGS = @MYSQL_CFLAGS@ @FFMPEG_CFLAGS@ -Wall -Wno-sign-compare -fno-inline
#AM_CXXFLAGS = -frepo

CLEANFILES = *.rpo

all_deps = ../*/*.a
all_libs = \
	-L../protocols -lprotocols \
	-L../encoders -lencoders \
	-L../processors -lprocessors \
	-L../consumers -lconsumers \
	-L../providers -lproviders \
	-L../base -lbase \
	-L../libimg -limg \
	-L../libgen -lgen

bin_PROGRAMS = \
	example1 \
	example2 \
	example3 \
	example4 \
	example5 \
	example6 \
	example7 \
	example8

example1_SOURCES = example1.cpp
example1_DEPENDENCIES = $(all_deps)
example1_LDADD = $(all_libs)

example2_SOURCES = example2.cpp
example2_DEPENDENCIES = $(all_deps)
example2_LDADD = $(all_libs)

example3_SOURCES = example3.cpp
example3_DEPENDENCIES = $(all_deps)
example3_LDADD = $(all_libs)

example4_SOURCES = example4.cpp
example4_DEPENDENCIES = $(all_deps)
example4_LDADD = $(all_libs)

example5_SOURCES = example5.cpp
example5_DEPENDENCIES = $(all_deps)
example5_LDADD = $(all_libs)

example6_SOURCES = example6.cpp
example6_DEPENDENCIES = $(all_deps)
example6_LDADD = $(all_libs)

example7_SOURCES = example7.cpp
example7_DEPENDENCIES = $(all_deps)
example7_LDADD = $(all_libs)

example8_SOURCES = example8.cpp
example8_DEPENDENCIES = $(all_deps)
example8_LDADD = $(all_libs)

noinst_HEADERS = 
