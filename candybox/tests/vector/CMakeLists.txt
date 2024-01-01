file(GLOB TEST_VECTOR_SOURCES ./*.cpp)
add_executable(test_vector ${TEST_VECTOR_SOURCES})
target_link_libraries(test_vector PRIVATE Catch2::Catch2)
target_include_directories(test_vector PRIVATE ../../../)
