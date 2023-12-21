file(GLOB TEST_OPTIONAL_SOURCES ./*.cpp)
add_executable(test_optional ${TEST_OPTIONAL_SOURCES})
target_link_libraries(test_optional PRIVATE candybox Catch2::Catch2)
