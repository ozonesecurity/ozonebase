# CMakeLists.txt for the ozone providers subfolder

set(OZ_PROVIDERS_SRC_FILES
    ozAVInput.cpp ozDummyInput.cpp ozLocalFileInput.cpp ozLocalVideoInput.cpp
    ozMemoryInput.cpp ozMemoryInputV1.cpp ozRawH264Input.cpp
    ozRemoteVideoInput.cpp ozSlaveVideo.cpp 
   )

set(OZ_PROVIDERS_HDR_FILES
    ozAVInput.h ozDummyInput.h ozLocalFileInput.h ozLocalVideoInput.h
    ozMemoryInput.h ozMemoryInputV1.h ozRawH264Input.h
    ozRemoteVideoInput.h ozSlaveVideo.h 
   )

if(OZ_V4L)
        list (APPEND OZ_PROVIDERS_SRC_FILES ozVideo4LinuxInput.cpp)
        list (APPEND OZ_PROVIDERS_HDR_FILES ozVideo4LinuxInput.h)
else(OZ_V4L)
        message (WARNING "**** Skipping V4L as it does not exist")
endif (OZ_V4L)

add_library(providers STATIC ${OZ_PROVIDERS_SRC_FILES} ${OZ_PROVIDERS_HDR_FILES})

target_link_libraries(providers ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS providers DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/providers")
install(FILES ${OZ_PROVIDERS_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/providers")

