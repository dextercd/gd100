#include <iostream>

#include "terminal.hpp"

int code_point_width(code_point)
{
    return 1;
}

void terminal::newline(int column)
{
    // TODO: handle bottom
    cursor.pos.y++;
    cursor.pos.x = column;
}

void terminal::write_char(code_point ch)
{
    auto width{code_point_width(ch)};

    auto* gl{glyph_at_cursor()};
    if (cursor.state.is_set(cursor_state_bit::wrap_next)) {
        gl->style.mode.set(glyph_attr_bit::text_wraps);
        newline();
        gl = glyph_at_cursor();
    }

    if (cursor.pos.x + width > screen.size().width) {
        newline();
        gl = glyph_at_cursor();
    }

    set_char(ch, cursor.style, cursor.pos);

    for (auto x{cursor.pos.x + 1};
         x < screen.size().width && x < cursor.pos.x + width;
         ++x)
    {
        gl[x].style.mode.set(glyph_attr_bit::wide);
    }

    if (cursor.pos.x + width < screen.size().width) {
        move_cursor_forward(width);
    } else {
        cursor.state.set(cursor_state_bit::wrap_next);
    }
}

void terminal::move_cursor_forward(int width)
{
    cursor.pos.x += width;
}

void terminal::set_char(code_point ch, glyph_style style, position pos)
{
    mark_dirty(pos.y);
    screen.get_glyph(pos) = {style, ch};
}

glyph* terminal::glyph_at_cursor()
{
    return &screen.get_glyph(cursor.pos);
}

void terminal::mark_dirty(int line)
{
}

void terminal::dump()
{
    for (int y = 0; y < screen.size().height; ++y) {
        for (int x = 0; x < screen.size().width; ++x) {
            auto pos = position{x, y};
            if (cursor.pos == pos) {
                std::cout << '_';
            } else {
                auto code = screen.get_glyph(pos).code;
                if (code == 0) {
                    std::cout << ' ';
                } else {
                    std::cout << (char)code;
                }
            }
        }
        std::cout << '\n';
    }
}
