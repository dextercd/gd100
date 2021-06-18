cmake_minimum_required(VERSION 3.19)

project(godot-terminal
    LANGUAGES CXX)

enable_testing()

add_subdirectory(character_width)
add_subdirectory(godot_lite_wrapper)


# terminal-interface

add_library(terminal-interface INTERFACE)

target_sources(terminal-interface
    INTERFACE
        src/terminal.cpp
        src/terminal_screen.cpp
        src/position.cpp
        src/terminal_decoder.cpp
        src/terminal_instructee.cpp)

target_include_directories(terminal-interface
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(terminal-interface PROPERTIES
    CXX_EXTENSIONS OFF
)

target_compile_features(terminal-interface
    INTERFACE cxx_std_17)

target_link_libraries(terminal-interface
    INTERFACE character_width)


# terminal-static

add_library(terminal-static STATIC
    $<TARGET_PROPERTY:terminal-interface,INTERFACE_SOURCES>)

target_include_directories(terminal-static
    PUBLIC $<TARGET_PROPERTY:terminal-interface,INTERFACE_INCLUDE_DIRECTORIES>)

target_compile_features(terminal-static
    PUBLIC $<TARGET_PROPERTY:terminal-interface,INTERFACE_COMPILE_FEATURES>)

target_link_libraries(terminal-static
    INTERFACE $<TARGET_PROPERTY:terminal-interface,INTERFACE_LINK_LIBRARIES>)


# godot-terminal

add_library(godot-terminal MODULE
    src/godot-export.cpp
    src/program_terminal_manager.cpp)

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
    PRIVATE
        terminal-interface
        godot-lite-wrapper)


add_subdirectory(extern/Catch2)
add_subdirectory(tests)

add_subdirectory(fuzzing)

add_subdirectory(terminfo)
