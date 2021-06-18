add_library(godot-lite-wrapper INTERFACE)

target_sources(godot-lite-wrapper
    INTERFACE
        src/api.cpp)

target_include_directories(godot-lite-wrapper
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(godot-lite-wrapper INTERFACE cxx_std_20)

