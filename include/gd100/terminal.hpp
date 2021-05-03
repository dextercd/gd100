#ifndef GDTERM_TERMINAL_HPP
#define GDTERM_TERMINAL_HPP

#include <cstdint>

#include "bit_container.hpp"
#include "terminal_screen.hpp"
#include "terminal_data.hpp"

namespace gd100 {

struct terminal_instruction;

class terminal {
public:
    terminal_cursor cursor{};
    terminal_screen screen{};
    terminal_mode mode{};

public:
    terminal() = default;
    terminal(extend screen_size)
        : screen{screen_size}
    {
    }

    void newline(bool first_column);
    void write_char(code_point ch);
    void move_cursor(position pos);
    void move_cursor_forward(int width);
    void set_char(code_point ch, glyph_style style, position pos);
    glyph* glyph_at_cursor();
    void mark_dirty(int line);
    void mark_dirty(int line_beg, int line_end);
    void scroll_up(int keep_top=0, int count=1);
    void scroll_down(int count=1);
    void clear_lines(int line_beg, int line_end);
    void clear(position start, position end);
    int process_bytes(const char* bytes, int length);
    void process_instruction(terminal_instruction inst);
    void delete_chars(int count);
    void dump();

private:
    position clamp_pos(position p);
};

} // gd100::

#endif // header guard
