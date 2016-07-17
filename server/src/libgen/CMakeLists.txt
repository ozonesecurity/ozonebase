# CMakeLists.txt for the ozone libgen subfolder

set(OZ_LIBGEN_SRC_FILES
    libgenBuffer.cpp libgenDebug.cpp libgenComms.cpp libgenRegexp.cpp
    libgenThread.cpp libgenTimer.cpp libgenTime.cpp libgenUtils.cpp
    libgenExec.cpp
   )

set(OZ_LIBGEN_HDR_FILES
    libgenBuffer.h libgenComms.h libgenDebug.h libgenException.h
    libgenExec.h libgenRegexp.h libgenThread.h libgenTimer.h 
    libgenTime.h libgenUtils.h
   )

add_library(gen STATIC ${OZ_LIBGEN_SRC_FILES} ${OZ_LIBGEN_HDR_FILES})

target_link_libraries(gen ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS gen DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/libgen")
install(FILES ${OZ_LIBGEN_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/libgen")

