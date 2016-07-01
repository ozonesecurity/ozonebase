# CMakeLists.txt for the ozone processors subfolder

set(OZ_PROCESSORS_SRC_FILES
    ozDelayBuffer.cpp ozFilterSwapUV.cpp ozImageConvert.cpp
    ozImageTimestamper.cpp ozMotionDetector.cpp ozMatrixVideo.cpp
    ozRateLimiter.cpp ozSignalChecker.cpp ozVideoFilter.cpp
   )

set(OZ_PROCESSORS_HDR_FILES
    ozDelayBuffer.h ozFilterSwapUV.h ozImageConvert.h ozImageTimestamper.h
    ozMotionDetector.h ozMatrixVideo.h ozRateLimiter.h ozSignalChecker.h
    ozVideoFilter.h
   )

add_library(processors ${OZ_PROCESSORS_SRC_FILES})

target_link_libraries(processors ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS processors DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/processors")
install(FILES ${OZ_PROCESSORS_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/processors")

