# CMakeLists.txt for the ozone libimg subfolder

set(OZ_LIBIMG_SRC_FILES
    libimgJpeg.c libimgPoly.cpp libimgImage.cpp
   )

set(OZ_LIBIMG_HDR_FILES
    libimgJinclude.h libimgJpeg.h libimgRgb.h libimgCoord.h
    libimgBox.h libimgPoly.h libimgImage.h
   )

add_library(img STATIC ${OZ_LIBIMG_SRC_FILES} ${OZ_LIBIMG_HDR_FILES})

target_link_libraries(img ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS})

install(TARGETS img DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/libimg")
install(FILES ${OZ_LIBIMG_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/libimg")

