# CMakeLists.txt for the ozone consumers subfolder

set(OZ_CONSUMER_SRC_FILES
    ozEventRecorder.cpp ozLocalFileDump.cpp ozLocalFileOutput.cpp 
    ozMemoryOutput.cpp ozMemoryOutputV1.cpp ozMemoryTriggerV1.cpp 
    ozMovieFileOutput.cpp ozMp4FileOutput.cpp ozVideoRecorder.cpp
   )

set(OZ_CONSUMER_HDR_FILES
    ozEventRecorder.h ozLocalFileDump.h ozLocalFileOutput.h 
    ozMemoryOutput.h ozMemoryOutputV1.h ozMemoryTriggerV1.h 
    ozMovieFileOutput.h ozMp4FileOutput.h ozVideoRecorder.h
   )

add_library(consumers STATIC ${OZ_CONSUMER_SRC_FILES} ${OZ_CONSUMER_HDR_FILES})

target_link_libraries(consumers ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS consumers DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/consumers")
install(FILES ${OZ_CONSUMER_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/consumers")

