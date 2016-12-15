# modify this to point to where ozone headers are
program_INCLUDE_DIRS := /home/arjunrc/ozoneroot/include/ozonebase
FFMPEG_INCLUDE_DIRS := /home/arjunrc/ozoneroot/include
# modify this to point to where ozone libs are
LIBPREFIX=/home/arjunrc/ozoneroot/lib/x86_64-linux-gnu/ozonebase
FFMPEGLIBPREFIX=/home/arjunrc/ozoneroot/lib

# You should not need to modify anything below this line
program_NAME := zm_monitor
program_CXX_SRCS := $(wildcard *.cpp)
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_CXX_OBJS)
program_LIBRARY_DIRS :=  $(LIBPREFIX)/base $(LIBPREFIX)/protocols $(LIBPREFIX)/libgen $(LIBPREFIX)/libimg $(LIBPREFIX)/encoders $(LIBPREFIX)/providers $(LIBPREFIX)/consumers $(LIBPREFIX)/processors
program_LIBRARY_DIRS += $(FFMPEGLIBPREFIX)

program_LIBRARIES := m protocols encoders processors consumers providers base img gen  jpeg pthread z avformat avcodec avdevice avutil avfilter swresample swscale dl cblas lapack dlib
program_HEADERDEPS = $(wildcard *.h)

CPPFLAGS += -g -std=gnu++11 
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
CPPFLAGS += -I$(FFMPEG_INCLUDE_DIRS)
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))
LDFLAGS +=  -Wl,-rpath,$(FFMPEGLIBPREFIX):

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS) ${program_HEADERDEPS}
	$(CXX) $(CPPFLAGS) $(program_OBJS) $(LDFLAGS) -o $(program_NAME)
	$(RM) *.o

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)

distclean: clean
