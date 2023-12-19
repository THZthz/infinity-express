message(STATUS "adding tests from ${CMAKE_CURRENT_SOURCE_DIR}/")
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
foreach (source_file ${TEST_SOURCES})
    get_filename_component(name ${source_file} NAME_WE ABSOLUTE)

    if (name STREQUAL "sp_bml_unique_ptr_test")
        continue()
    endif ()

    # should failed to compile
    if (name MATCHES "fail")
        continue()
    endif ()

    # special cases
    if (name STREQUAL "dll_test_lib" OR name STREQUAL "dll_test_main" OR
            name STREQUAL "abi_test_lib" OR name STREQUAL "abi_test_main" OR
            name STREQUAL "collector_test" OR name STREQUAL "yield_k_test" OR
            name STREQUAL "yield_k_windows_h_test")
        continue()
    endif ()

    # common cases
    message(STATUS "adding ${name} : ${source_file}")
    add_executable(${name} ${source_file})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/utils)
    add_test(${name} ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 30)
endforeach ()
