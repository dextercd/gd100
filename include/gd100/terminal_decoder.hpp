#ifndef GDTERM_TERMINAL_DECODER_HPP
#define GDTERM_TERMINAL_DECODER_HPP

#include <string>

#include "glyph.hpp"
#include "position.hpp"
#include "terminal_data.hpp"

namespace gd100 {

enum class direction {
    up, down, forward, back
};

class decoder_instructee {
public:
    virtual void tab() = 0;
    virtual void line_feed(bool first_col) = 0;
    virtual void carriage_return() = 0;
    virtual void backspace() = 0;
    virtual void write_char(code_point code) = 0;
    virtual void clear_to_bottom() = 0;
    virtual void clear_from_top() = 0;
    virtual void clear_screen() = 0;
    virtual void clear_to_end() = 0;
    virtual void clear_from_begin() = 0;
    virtual void clear_line() = 0;
    virtual void position_cursor(position pos) = 0;
    virtual void change_mode_bits(bool set, terminal_mode mode) = 0;
    virtual void move_cursor(int count, direction dir, bool first_col=false) = 0;
    virtual void move_to_column(int column) = 0;
    virtual void move_to_row(int row) = 0;
    virtual void delete_chars(int count) = 0;
    virtual void erase_chars(int count) = 0;
    virtual void delete_lines(int count) = 0;
    virtual void reverse_line_feed() = 0;
    virtual void insert_blanks(int count) = 0;
    virtual void insert_newline(int count) = 0;
    virtual void set_charset_table(int table_index, charset cs) = 0;
    virtual void use_charset_table(int table_index) = 0;
    virtual void reset_style() = 0;
    virtual void set_foreground(colour c) = 0;
    virtual void set_background(colour c) = 0;
    virtual void set_reversed(bool enable) = 0;
    virtual void default_foreground() = 0;
    virtual void default_background() = 0;
    virtual void set_bold(bool enable) = 0;
    virtual void set_mouse_mode(mouse_mode, bool set) = 0;
    virtual void set_mouse_mode_extended(bool set) = 0;
    virtual void set_bracketed_paste(bool set) = 0;
};

class decoder {
private:
    std::string buffer;

public:
    void decode(char const* bytes, int count, decoder_instructee& t);
};

} // gd100::

#endif // header guard
