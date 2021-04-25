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
};

struct decode_result {
    int bytes_consumed;
    terminal_instruction instruction;
};

decode_result decode(char const* bytes, int count);

} // gd100::

#endif // header guard
