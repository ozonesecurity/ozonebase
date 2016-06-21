# CMakeLists.txt for the ozone libgen subfolder

# If this is an out-of-source build, copy the files we need to the binary directory
if(NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR))
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libgenThreadData.cpp" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif(NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR))

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

add_library(gen ${OZ_LIBGEN_SRC_FILES})

target_link_libraries(gen ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS gen DESTINATION lib)
install(FILES ${OZ_LIBGEN_HDR_FILES} DESTINATION include)

