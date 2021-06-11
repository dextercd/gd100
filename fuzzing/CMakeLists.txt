add_executable(fuzz_test
    fuzztest.cpp)

target_link_libraries(fuzz_test PRIVATE terminal-interface)

target_compile_options(fuzz_test PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(fuzz_test PRIVATE -fsanitize=fuzzer,address,undefined)
