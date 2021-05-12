#ifndef GDTERM_TERMINAL_DECODER_HPP
#define GDTERM_TERMINAL_DECODER_HPP

#include "glyph.hpp"
#include "position.hpp"
#include "terminal_data.hpp"

namespace gd100 {

enum class instruction_type {
    none,
    backspace,
    line_feed,
    carriage_return,
    write_char,

    clear_to_bottom,
    clear_from_top,
    clear_screen,

    clear_to_end,
    clear_from_begin,
    clear_line,

    move_to_column,
    move_to_row,
    position_cursor,
    move_cursor,

    change_mode_bits,

    delete_chars,
    erase_chars,
    delete_lines,

    reverse_line_feed,

    insert_blanks,
    insert_newline,

    set_charset_table,
    use_charset_table,
};

enum class direction {
    up, down, forward, back
};

template<class I>
auto instruction_number = nullptr;

struct none_instruction {};

template<> constexpr inline
auto instruction_number<none_instruction> = instruction_type::none;

struct backspace_instruction {};
template<> constexpr inline
auto instruction_number<backspace_instruction> = instruction_type::backspace;

struct line_feed_instruction {};
template<> constexpr inline
auto instruction_number<line_feed_instruction> = instruction_type::line_feed;

struct carriage_return_instruction {};
template<> constexpr inline
auto instruction_number<carriage_return_instruction> = instruction_type::carriage_return;

struct write_char_instruction {
    code_point code;
};
template<> constexpr inline
auto instruction_number<write_char_instruction> = instruction_type::write_char;

struct clear_to_bottom_instruction {};
template<> constexpr inline
auto instruction_number<clear_to_bottom_instruction> = instruction_type::clear_to_bottom;

struct clear_from_top_instruction {};
template<> constexpr inline
auto instruction_number<clear_from_top_instruction> = instruction_type::clear_from_top;

struct clear_screen_instruction {};
template<> constexpr inline
auto instruction_number<clear_screen_instruction> = instruction_type::clear_screen;

struct clear_to_end_instruction {};
template<> constexpr inline
auto instruction_number<clear_to_end_instruction> = instruction_type::clear_to_end;

struct clear_from_begin_instruction {};
template<> constexpr inline
auto instruction_number<clear_from_begin_instruction> = instruction_type::clear_from_begin;

struct clear_line_instruction {};
template<> constexpr inline
auto instruction_number<clear_line_instruction> = instruction_type::clear_line;

struct position_cursor_instruction {
    position pos;
};
template<> constexpr inline
auto instruction_number<position_cursor_instruction> = instruction_type::position_cursor;

struct move_cursor_instruction {
    int count;
    direction dir;
};
template<> constexpr inline
auto instruction_number<move_cursor_instruction> = instruction_type::move_cursor;

struct move_to_column_instruction {
    int column;
};
template<> constexpr inline
auto instruction_number<move_to_column_instruction> = instruction_type::move_to_column;

struct move_to_row_instruction {
    int row;
};
template<> constexpr inline
auto instruction_number<move_to_row_instruction> = instruction_type::move_to_row;

struct change_mode_bits_instruction {
    bool set;
    terminal_mode mode;
};
template<> constexpr inline
auto instruction_number<change_mode_bits_instruction> = instruction_type::change_mode_bits;

struct delete_chars_instruction {
    int count;
};
template<> constexpr inline
auto instruction_number<delete_chars_instruction> = instruction_type::delete_chars;

struct erase_chars_instruction {
    int count;
};
template<> constexpr inline
auto instruction_number<erase_chars_instruction> = instruction_type::erase_chars;

struct delete_lines_instruction {
    int count;
};
template<> constexpr inline
auto instruction_number<delete_lines_instruction> = instruction_type::delete_lines;

struct reverse_line_feed_instruction {};
template<> constexpr inline
auto instruction_number<reverse_line_feed_instruction> = instruction_type::reverse_line_feed;

struct insert_blanks_instruction {
    int count;
};
template<> constexpr inline
auto instruction_number<insert_blanks_instruction> = instruction_type::insert_blanks;

struct insert_newline_instruction {
    int count;
};
template<> constexpr inline
auto instruction_number<insert_newline_instruction> = instruction_type::insert_newline;

struct set_charset_table_instruction {
    int table_index;
    charset cs;
};
template<> constexpr inline
auto instruction_number<set_charset_table_instruction> = instruction_type::set_charset_table;

struct use_charset_table_instruction {
    int table_index;
};
template<> constexpr inline
auto instruction_number<use_charset_table_instruction> = instruction_type::use_charset_table;

struct terminal_instruction {
    instruction_type type;

    template<class I>
    terminal_instruction(I instruction)
    {
        type = instruction_number<I>;
        set_data(instruction);
    }

    union /* data */ {
        write_char_instruction write_char;
        position_cursor_instruction position_cursor;
        change_mode_bits_instruction change_mode_bits;
        move_cursor_instruction move_cursor;
        move_to_column_instruction move_to_column;
        move_to_row_instruction move_to_row;
        delete_chars_instruction delete_chars;
        erase_chars_instruction erase_chars;
        delete_lines_instruction delete_lines;
        insert_blanks_instruction insert_blanks;
        insert_newline_instruction insert_newline;
        set_charset_table_instruction set_charset_table;
        use_charset_table_instruction use_charset_table;
    };

private:
    void set_data(none_instruction) {}
    void set_data(line_feed_instruction) {}
    void set_data(carriage_return_instruction) {}
    void set_data(backspace_instruction) {}

    void set_data(write_char_instruction i)
    {
        write_char = i;
    }

    void set_data(clear_to_bottom_instruction) {}
    void set_data(clear_from_top_instruction) {}
    void set_data(clear_screen_instruction) {}

    void set_data(clear_to_end_instruction) {}
    void set_data(clear_from_begin_instruction) {}
    void set_data(clear_line_instruction) {}

    void set_data(position_cursor_instruction i)
    {
        position_cursor = i;
    }

    void set_data(change_mode_bits_instruction i)
    {
        change_mode_bits = i;
    }

    void set_data(move_cursor_instruction i)
    {
        move_cursor = i;
    }

    void set_data(move_to_column_instruction i)
    {
        move_to_column = i;
    }

    void set_data(move_to_row_instruction i)
    {
        move_to_row = i;
    }

    void set_data(delete_chars_instruction i)
    {
        delete_chars = i;
    }

    void set_data(erase_chars_instruction i)
    {
        erase_chars = i;
    }

    void set_data(delete_lines_instruction i)
    {
        delete_lines = i;
    }

    void set_data(reverse_line_feed_instruction) {}

    void set_data(insert_blanks_instruction i)
    {
        insert_blanks = i;
    }

    void set_data(insert_newline_instruction i)
    {
        insert_newline = i;
    }

    void set_data(set_charset_table_instruction i)
    {
        set_charset_table = i;
    }

    void set_data(use_charset_table_instruction i)
    {
        use_charset_table = i;
    }
};

struct decode_result {
    int bytes_consumed;
    terminal_instruction instruction;
};

decode_result decode(char const* bytes, int count);

} // gd100::

#endif // header guard
