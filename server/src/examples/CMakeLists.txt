# CMakeLists.txt for the ozone examples subfolder

add_executable(example1 example1.cpp)
add_executable(example2 example2.cpp)
add_executable(example3 example3.cpp)
add_executable(example4 example4.cpp)
add_executable(example5 example5.cpp)
add_executable(example6 example6.cpp)
add_executable(example7 example7.cpp)
add_executable(example8 example8.cpp)
add_executable(example9 example9.cpp)
add_executable(example10 example10.cpp)
add_executable(example11 example11.cpp)
add_executable(starter_example starter_example.cpp)
add_executable(nvrcli/nvrcli nvrcli/nvrcli.cpp nvrcli/nvrNotifyOutput.cpp)

set (EXAMPLES example1 example2 example3 example4 example5 example6 example7 example8 example9 example10 example11 starter_example)

if(MACOSX)
        add_executable(example_osx example_osx.cpp)
        target_link_libraries(example_osx ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
        list(APPEND EXAMPLES "example_osx")
endif(MACOSX)

target_link_libraries(example1 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example2 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example3 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example4 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example5 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example6 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example7 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example8 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example9 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example10 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(example11 ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(starter_example ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})
target_link_libraries(nvrcli/nvrcli ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})



install(
        TARGETS ${EXAMPLES}
       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
       )


