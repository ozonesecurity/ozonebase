AUTOMAKE_OPTIONS = gnu

AM_CPPFLAGS = -Wall -Wno-sign-compare -fno-inline
#AM_CXXFLAGS = -frepo
AM_LDFLAGS = \
    -L../protocols \
    -L../encoders \
    -L../processors \
    -L../consumers \
    -L../providers \
    -L../base \
    -L../libimg \
    -L../libgen

CLEANFILES = *.rpo

all_deps = ../*/*.a
all_libs = \
	-lprotocols \
	-lencoders \
	-lprocessors \
	-lconsumers \
	-lproviders \
	-lbase \
	-limg \
	-lgen

AM_DEPENDENCIES = $(all_deps)
LDADD = $(all_libs)

bin_PROGRAMS = \
	example1 \
	example2 \
	example3 \
	example4 \
	example5 \
	example6 \
	example7 \
	example8 \
	starter_example

example1_SOURCES = example1.cpp
example1_DEPENDENCIES = $(all_deps)

example2_SOURCES = example2.cpp
example2_DEPENDENCIES = $(all_deps)

example3_SOURCES = example3.cpp
example3_DEPENDENCIES = $(all_deps)

example4_SOURCES = example4.cpp
example4_DEPENDENCIES = $(all_deps)

example5_SOURCES = example5.cpp
example5_DEPENDENCIES = $(all_deps)

example6_SOURCES = example6.cpp
example6_DEPENDENCIES = $(all_deps)

example7_SOURCES = example7.cpp
example7_DEPENDENCIES = $(all_deps)

example8_SOURCES = example8.cpp
example8_DEPENDENCIES = $(all_deps)

starter_example_SOURCES = starter_example.cpp
starter_example_DEPENDENCIES = $(all_deps)

noinst_HEADERS = 
