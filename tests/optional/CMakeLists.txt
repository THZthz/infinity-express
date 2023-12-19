file(GLOB SOURCES ./*.cpp)
add_executable(test_optional ${SOURCES})
target_include_directories(test_optional PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/utils ${CMAKE_CURRENT_SOURCE_DIR}/../../include/)
target_link_libraries(test_optional PRIVATE Catch2::Catch2)
