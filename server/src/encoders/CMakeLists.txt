# CMakeLists.txt for the ozone encoders subfolder

set(OZ_ENCODER_SRC_FILES
    ozH264Encoder.cpp ozH264Relay.cpp ozMpegEncoder.cpp ozJpegEncoder.cpp
   )

set(OZ_ENCODER_HDR_FILES
    ozH264Encoder.h ozH264Relay.h ozMpegEncoder.h ozJpegEncoder.h
   )

add_library(encoders ${OZ_ENCODER_SRC_FILES})

target_link_libraries(encoders ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS encoders DESTINATION lib)
install(FILES ${OZ_ENCODER_HDR_FILES} DESTINATION include)

