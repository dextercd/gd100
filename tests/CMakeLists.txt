add_executable(test_runner
    test_main.cpp

    terminal.cpp
    decoding.cpp
    regressions.cpp)

target_link_libraries(test_runner
    PRIVATE Catch2::Catch2
    PRIVATE terminal-static)

add_test(NAME test_runner COMMAND test_runner)
