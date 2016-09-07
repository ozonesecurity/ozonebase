# CMakeLists.txt for the ozone providers subfolder

set(OZ_PROVIDERS_SRC_FILES
    ozAVInput.cpp ozLocalFileInput.cpp ozLocalVideoInput.cpp
    ozMemoryInput.cpp ozMemoryInputV1.cpp ozRawH264Input.cpp
    ozRemoteVideoInput.cpp ozSlaveVideo.cpp ozVideo4LinuxInput.cpp
   )

set(OZ_PROVIDERS_HDR_FILES
    ozAVInput.h ozLocalFileInput.h ozLocalVideoInput.h
    ozMemoryInput.h ozMemoryInputV1.h ozRawH264Input.h
    ozRemoteVideoInput.h ozSlaveVideo.h ozVideo4LinuxInput.h
   )

add_library(providers STATIC ${OZ_PROVIDERS_SRC_FILES} ${OZ_PROVIDERS_HDR_FILES})

target_link_libraries(providers ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS providers DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/providers")
install(FILES ${OZ_PROVIDERS_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/providers")

