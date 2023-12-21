enable_testing()

add_subdirectory(smart_ptr)
add_subdirectory(FSM)

add_executable(test_bitwise_enum ./test_bitwise_enum.cpp)
target_link_libraries(test_bitwise_enum PRIVATE candybox)
add_test(test_bitwise_enum test_bitwise_enum)

add_executable(test_spatial ./tests_spatial.cpp)
target_link_libraries(test_spatial PRIVATE candybox)
add_test(test_spatial test_spatial)

add_executable(test_vector ./tests_vector.cpp)
target_link_libraries(test_vector PRIVATE candybox)
add_test(test_vector test_vector)

add_subdirectory(optional)

