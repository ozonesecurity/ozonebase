# CMakeLists.txt for the ozone base subfolder

set(OZ_BASE_SRC_FILES
    ozAlarmFrame.cpp ozApp.cpp ozConfig.cpp ozConnection.cpp
    ozController.cpp ozEncoder.cpp ozFeedBase.cpp ozFeedConsumer.cpp
    ozFeedFrame.cpp ozFeedProvider.cpp ozFfmpeg.cpp ozListener.cpp
    ozMemoryIO.cpp ozMemoryIOV1.cpp ozMotionData.cpp ozMotionFrame.cpp
    ozNotifyFrame.cpp ozRegion.cpp ozSignal.cpp ozStream.cpp ozZone.cpp
   )

set(OZ_BASE_HDR_FILES
    countedPtr.h linkedPtr.h ozAlarmFrame.h ozApp.h ozConfigDefines.h
    ozConfig.h ozConnection.h ozController.h ozEncoder.h ozFeedBase.h
    ozFeedConsumer.h ozFeedFrame.h ozFeedProvider.h ozFfmpeg.h oz.h
    ozListener.h ozMemoryIO.h ozMemoryIOV1.h ozMotionData.h ozMotionFrame.h
    ozNotifyFrame.h ozRegion.h ozSignal.h ozStream.h ozZone.h
   )

add_library(base ${OZ_BASE_SRC_FILES})

target_link_libraries(base ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS base DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/base")
install(FILES ${OZ_BASE_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/base")

