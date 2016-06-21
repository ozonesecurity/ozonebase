# CMakeLists.txt for the ozone consumers subfolder

set(OZ_CONSUMER_SRC_FILES
    ozEventRecorder.cpp ozLocalFileDump.cpp ozLocalFileOutput.cpp 
    ozMemoryOutput.cpp ozMovieFileOutput.cpp ozMp4FileOutput.cpp
   )

set(OZ_CONSUMER_HDR_FILES
    ozEventRecorder.h ozLocalFileDump.h ozLocalFileOutput.h 
    ozMemoryOutput.h ozMovieFileOutput.h ozMp4FileOutput.h
   )

add_library(consumers ${OZ_CONSUMER_SRC_FILES})

target_link_libraries(consumers ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS consumers DESTINATION lib)
install(FILES ${OZ_CONSUMER_HDR_FILES} DESTINATION include)

