#ifndef GDTERM_TERMINAL_HPP
#define GDTERM_TERMINAL_HPP

#include <cstdint>

#include "bit_container.hpp"
#include "terminal_screen.hpp"
#include "terminal_data.hpp"
#include "terminal_decoder.hpp"

namespace gd100 {

class terminal {
    friend struct terminal_instructee;

public:
    terminal_cursor cursor{};
    terminal_screen screen{};
    terminal_mode mode{};
    mouse_mode mouse{};

private:
    charset translation_tables[4] = {
        charset::usa, charset::usa,
        charset::usa, charset::usa};
    int using_translation_table = 0;

public:
    terminal() = default;
    terminal(extend screen_size)
        : screen{screen_size}
    {
    }

    charset current_charset() const;

    void tab();
    void newline(bool first_column);
    void write_char(code_point ch);
    void move_cursor(position pos);
    void move_cursor_forward(int width);
    void set_char(code_point ch, int width, glyph_style style, position pos);
    glyph* glyph_at_cursor();
    void mark_dirty(int line);
    void mark_dirty(int line_beg, int line_end);
    void scroll_up(int keep_top=0, int count=1);
    void scroll_down(int keep_top=0, int count=1);
    void clear_lines(int line_beg, int line_end);
    void clear(position start, position end);
    void delete_chars(int count);
    void insert_blanks(int count);
    void insert_newline(int count);
    void reset_style();
    void dump();

private:
    position clamp_pos(position p);
};

struct terminal_instructee : decoder_instructee {
    terminal* term;

    terminal_instructee(terminal* tm)
        : term{tm}
    {
    }

    void tab() override;
    void line_feed(bool first_col) override;
    void carriage_return() override;
    void backspace() override;
    void write_char(code_point code) override;
    void clear_to_bottom() override;
    void clear_from_top() override;
    void clear_screen() override;
    void clear_to_end() override;
    void clear_from_begin() override;
    void clear_line() override;
    void position_cursor(position pos) override;
    void change_mode_bits(bool set, terminal_mode mode) override;
    void move_cursor(int count, direction dir, bool first_col) override;
    void move_to_column(int column) override;
    void move_to_row(int row) override;
    void delete_chars(int count) override;
    void erase_chars(int count) override;
    void delete_lines(int count) override;
    void reverse_line_feed() override;
    void insert_blanks(int count) override;
    void insert_newline(int count) override;
    void set_charset_table(int table_index, charset cs) override;
    void use_charset_table(int table_index) override;
    void reset_style() override;
    void set_foreground(colour c) override;
    void set_background(colour c) override;
    void set_reversed(bool enable) override;
    void default_foreground() override;
    void default_background() override;
    void set_bold(bool enable) override;
    void set_mouse_mode(mouse_mode, bool set) override;
    void set_mouse_mode_extended(bool set) override;
    void set_bracketed_paste(bool set) override;
};


} // gd100::

#endif // header guard
