# CMakeLists.txt for the ozone base subfolder

set(OZ_BASE_SRC_FILES
    ozAlarmFrame.cpp ozApp.cpp ozConfig.cpp ozConnection.cpp
    ozController.cpp ozEncoder.cpp ozFeedBase.cpp ozFeedConsumer.cpp
    ozFeedFrame.cpp ozFeedProvider.cpp ozFfmpeg.cpp ozFilter.cpp ozListener.cpp
    ozMemoryIO.cpp ozMemoryIOV1.cpp ozMotionData.cpp ozMotionFrame.cpp
    ozNotifyFrame.cpp ozOptions.cpp ozParameters.cpp ozRegion.cpp ozSignal.cpp
    ozStream.cpp ozZone.cpp ozDetector.cpp ozRecorder.cpp
   )

set(OZ_BASE_HDR_FILES
    countedPtr.h linkedPtr.h ozAlarmFrame.h ozApp.h ozConfigDefines.h
    ozConnection.h ozController.h ozEncoder.h ozFeedBase.h
    ozFeedConsumer.h ozFeedFrame.h ozFeedProvider.h ozFfmpeg.h ozFilter.h oz.h
    ozListener.h ozMemoryIO.h ozMemoryIOV1.h ozMotionData.h ozMotionFrame.h
    ozNotifyFrame.h ozOptions.h ozParameters.h ozRegion.h ozSignal.h
    ozStream.h ozZone.h ozDetector.h ozRecorder.h
    "${CMAKE_CURRENT_BINARY_DIR}/ozConfig.h"
   )

add_library(base STATIC ${OZ_BASE_SRC_FILES} ${OZ_BASE_HDR_FILES})

target_link_libraries(base ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS base DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/base")
install(FILES ${OZ_BASE_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/base")

