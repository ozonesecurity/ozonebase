# CMakeLists.txt for the ozone protocols subfolder

set(OZ_PROTOCOLS_SRC_FILES
    ozHttp.cpp ozHttpConnection.cpp ozHttpController.cpp ozHttpSession.cpp
    ozHttpStream.cpp ozRtmp.cpp ozRtmpAmf.cpp ozRtmpConnection.cpp
    ozRtmpController.cpp ozRtmpRequest.cpp ozRtmpSession.cpp ozRtmpStream.cpp
    ozRtsp.cpp ozRtspConnection.cpp ozRtspController.cpp ozRtspSession.cpp
    ozRtspStream.cpp ozRtpCtrl.cpp ozRtpData.cpp ozRtpSession.cpp
   )

set(OZ_PROTOCOLS_HDR_FILES
    ozHttp.h ozHttpConnection.h ozHttpController.h ozHttpSession.h
    ozHttpStream.h ozRtmp.h ozRtmpAmf.h ozRtmpConnection.h
    ozRtmpController.h ozRtmpRequest.h ozRtmpSession.h ozRtmpStream.h
    ozRtsp.h ozRtspConnection.h ozRtspController.h ozRtspSession.h
    ozRtspStream.h ozRtpCtrl.h ozRtpData.h ozRtpSession.h
   )

add_library(protocols ${OZ_PROTOCOLS_SRC_FILES})

target_link_libraries(protocols ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS protocols DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/protocols")
install(FILES ${OZ_PROTOCOLS_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/protocls")

