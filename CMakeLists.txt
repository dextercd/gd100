cmake_minimum_required(VERSION 3.19)

project(godot-terminal
    LANGUAGES CXX)

add_subdirectory(extern/katerm)

add_subdirectory(godot_lite_wrapper)
add_subdirectory(godot_terminal)
