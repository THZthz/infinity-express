file(GLOB SOURCES ./*.cpp)
add_executable(test_smart_ptr2 ${SOURCES})
target_include_directories(test_smart_ptr2 PRIVATE ../../include)
target_link_libraries(test_smart_ptr2 PRIVATE Catch2::Catch2)

