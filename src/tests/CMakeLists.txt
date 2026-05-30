set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

file(GLOB CONFIGURE_DEPENDS cpp_lib_files
    "${SRC_ROOT}/utils/*.cpp"
    "${SRC_ROOT}/abstract_syntax_tree/*.cpp"
    "${SRC_ROOT}/abstract_syntax_tree/nodes/*.cpp"
    "${SRC_ROOT}/abstract_syntax_tree/tokens/*.cpp"
    "${SRC_ROOT}/script_runtime/*.cpp"
)

file(GLOB test_files
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