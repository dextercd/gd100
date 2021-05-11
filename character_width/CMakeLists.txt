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
        generate_double_width_table.py
        parse_eaw.py
        range_list.py
)

add_library(character_width
    ${CMAKE_CURRENT_BINARY_DIR}/double_width_table.inc
    src/character_width.cpp)

target_include_directories(character_width
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC include)
