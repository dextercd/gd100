if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/EastAsianWidth.txt)
    file(DOWNLOAD
        https://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
        ${CMAKE_CURRENT_BINARY_DIR}/EastAsianWidth.txt)
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/double_width_table.inc
    COMMAND
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/generate_double_width_table.py
                --input ${CMAKE_CURRENT_BINARY_DIR}/EastAsianWidth.txt
                --output ${CMAKE_CURRENT_BINARY_DIR}/double_width_table.inc

    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/EastAsianWidth.txt
)


if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/DerivedGeneralCategory.txt)
    file(DOWNLOAD
        https://www.unicode.org/Public/UCD/latest/ucd/extracted/DerivedGeneralCategory.txt
        ${CMAKE_CURRENT_BINARY_DIR}/DerivedGeneralCategory.txt)
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zero_width_table.inc
    COMMAND
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/generate_zero_width_table.py
                --input ${CMAKE_CURRENT_BINARY_DIR}/DerivedGeneralCategory.txt
                --output ${CMAKE_CURRENT_BINARY_DIR}/zero_width_table.inc

    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/DerivedGeneralCategory.txt
)

add_library(character_width INTERFACE)

target_sources(character_width
    INTERFACE
        ${CMAKE_CURRENT_BINARY_DIR}/double_width_table.inc
        ${CMAKE_CURRENT_BINARY_DIR}/zero_width_table.inc
        src/character_width.cpp)

target_include_directories(character_width
    INTERFACE ${CMAKE_CURRENT_BINARY_DIR}
    INTERFACE include)
