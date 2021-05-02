#ifndef GDTERM_TERMINAL_DECODER_HPP
#define GDTERM_TERMINAL_DECODER_HPP

#include "glyph.hpp"

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

    position_cursor,
};

template<class I>
instruction_type instruction_number;

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
};

struct decode_result {
    int bytes_consumed;
    terminal_instruction instruction;
};

decode_result decode(char const* bytes, int count);

} // gd100::

#endif // header guard
