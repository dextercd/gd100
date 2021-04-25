cmake_minimum_required(VERSION 3.10)

project(godot-terminal
    LANGUAGES CXX)

enable_testing()

add_library(terminal STATIC
    src/terminal.cpp
    src/terminal_screen.cpp
    src/position.cpp)

target_include_directories(terminal
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(terminal PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON # Needed to link into shared library
)

target_compile_features(terminal
    PRIVATE cxx_std_17)


add_library(godot-terminal MODULE
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

target_link_libraries(godot-terminal
    PRIVATE terminal)


add_subdirectory(extern/Catch2)

add_subdirectory(tests)
