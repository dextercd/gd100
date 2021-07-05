add_library(godot-terminal MODULE
    src/godot-export.cpp
    src/program_terminal_manager.cpp)

include(GenerateExportHeader)
generate_export_header(godot-terminal
    BASE_NAME gdterm)

target_include_directories(godot-terminal
    PRIVATE ${CMAKE_SOURCE_DIR}/extern/godot-headers/
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
