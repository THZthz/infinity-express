enable_testing()

add_subdirectory(smart_ptr)
add_subdirectory(smart_ptr2)

add_executable(test_bitwise_enum ./test_bitwise_enum.cpp)
target_include_directories(test_bitwise_enum PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
add_test(test_bitwise_enum test_bitwise_enum)

add_executable(test_spatial ./tests_spatial.cpp)
target_include_directories(test_spatial PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
target_link_libraries(test_spatial PRIVATE glm)
add_test(test_spatial test_spatial)

add_executable(test_vector ./tests_vector.cpp)
target_include_directories(test_vector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
add_test(test_vector test_vector)

add_subdirectory(optional)

