cmake_minimum_required(VERSION 3.10)

project(godot-terminal
    LANGUAGES CXX)

add_library(godot-terminal MODULE
    src/terminal.cpp
    src/godot-export.cpp)

include(GenerateExportHeader)
generate_export_header(godot-terminal
    BASE_NAME gdterm)

target_include_directories(godot-terminal
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-headers/
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(godot-terminal PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

target_compile_features(godot-terminal
    PRIVATE cxx_std_17)
