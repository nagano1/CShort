set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

file(GLOB cpp_lib_files CONFIGURE_DEPENDS
    "${SRC_ROOT}/utils/*.cpp"
    "${SRC_ROOT}/parser/*.cpp"
    "${SRC_ROOT}/parser/nodes/*.cpp"
    "${SRC_ROOT}/parser/tokens/*.cpp"
    "${SRC_ROOT}/parser/validation/*.cpp"
    "${SRC_ROOT}/script_engine/*.cpp"
    "${SRC_ROOT}/compiler/*.cpp"
)

file(GLOB test_files CONFIGURE_DEPENDS
    "${SRC_ROOT}/tests/*.cpp"
)

add_library(common_lib STATIC ${cpp_lib_files})

foreach (file ${test_files})
    get_filename_component(filename ${file} NAME_WE)
    add_executable(${filename}_run ${file})
    target_link_libraries(${filename}_run common_lib)
    install(TARGETS ${filename}_run RUNTIME DESTINATION bin)
endforeach()


#find_package(Threads REQUIRED)
#target_link_libraries(${BINARY}_run Threads::Threads)