#include <iostream>
#include <algorithm>

#include <gd100/terminal.hpp>
#include <gd100/terminal_decoder.hpp>
#include <cw/character_width.hpp>

namespace gd100 {

charset terminal::current_charset() const
{
    return translation_tables[using_translation_table];
}

void terminal::newline(bool first_column)
{
    if (cursor.pos.y + 1 < screen.size().height) {
        cursor.pos.y++;
    } else {
        scroll_up(0, 1);
    }

    if (first_column)
        cursor.pos.x = 0;
}

void terminal::write_char(code_point ch)
{
    auto width{cw::character_width(ch)};
    if (width == -1)
        return;

    auto* gl{glyph_at_cursor()};
    if (cursor.state.is_set(cursor_state_bit::wrap_next)) {
        gl->style.mode.set(glyph_attr_bit::text_wraps);
        newline(true);
        gl = glyph_at_cursor();
    }

    if (mode.is_set(terminal_mode_bit::insert)) {
        std::move_backward(
            screen.get_line(cursor.pos.y) + cursor.pos.x,
            screen.get_line(cursor.pos.y) + screen.size().width - width,
            screen.get_line(cursor.pos.y) + screen.size().width);
    }

    if (cursor.pos.x + width > screen.size().width) {
        newline(true);
        gl = glyph_at_cursor();
    }

    set_char(ch, cursor.style, cursor.pos);

    if (width == 2) {
        gl[0].style.mode.set(glyph_attr_bit::wide);
        if (cursor.pos.x + 1 < screen.size().width) {
            gl[1].style.mode = glyph_attribute{glyph_attr_bit::wdummy};
        }
    }

    if (cursor.pos.x + width < screen.size().width) {
        move_cursor_forward(width);
    } else {
        cursor.state.set(cursor_state_bit::wrap_next);
    }
}

void terminal::move_cursor(position pos)
{
    cursor.pos = clamp_pos(pos);
    cursor.state.unset(cursor_state_bit::wrap_next);
}

void terminal::move_cursor_forward(int width)
{
    auto new_pos = cursor.pos;
    new_pos.x += width;
    move_cursor(new_pos);
}

void terminal::set_char(code_point ch, glyph_style style, position pos)
{
    // The table is proudly stolen from st from rxvt.
    static char32_t vt100_0[] = {
        U'↑', U'↓', U'→', U'←', U'█', U'▚', U'☃',
        0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   ,
        0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   ,
        0   , 0   , 0   , 0   , 0   , 0   , 0   , U' ',
        U'◆', U'▒', U'␉', U'␌', U'␍', U'␊', U'°', U'±',
        U'␤', U'␋', U'┘', U'┐', U'┌', U'└', U'┼', U'⎺',
        U'⎻', U'─', U'⎼', U'⎽', U'├', U'┤', U'┴', U'┬',
        U'│', U'≤', U'≥', U'π', U'≠', U'£', U'·',
    };

    if (current_charset() == charset::graphic0 &&
        ch >= 0x41 && ch <= 0x7e && vt100_0[ch] != 0)
    {
        ch = vt100_0[ch - 0x41];
    }

    mark_dirty(pos.y);
    screen.get_glyph(pos) = {style, ch};
    // TODO: implement wide character overwriting properly.
}

glyph* terminal::glyph_at_cursor()
{
    return &screen.get_glyph(cursor.pos);
}

void terminal::mark_dirty(int line)
{
}

void terminal::mark_dirty(int line_beg, int line_end)
{
}

void terminal::scroll_up(int keep_top, int count)
{
    int move_to = std::clamp(keep_top, 0, screen.size().height);
    int move_start = std::clamp(move_to + count, 0, screen.size().height);
    int move_end = screen.size().height;

    std::move(
        screen.get_line(move_start),
        screen.get_line(move_end), // pointer past last glyph
        screen.get_line(move_to));

    clear_lines(move_end - count, move_end);

    mark_dirty(move_to, move_end);
}

void terminal::scroll_down(int keep_top, int count)
{
    auto const height = screen.size().height;

    keep_top = std::clamp(keep_top, 0, height);
    count = std::clamp(count, 0, height - keep_top);

    auto const move_count = height - keep_top - count;
    auto const line_end = keep_top + move_count;

    std::move_backward(
        screen.get_line(keep_top),
        screen.get_line(line_end),
        screen.get_line(height));

    clear_lines(keep_top, keep_top + count);

    mark_dirty(keep_top, height);
}

void terminal::clear_lines(int line_beg, int line_end)
{
    line_end = std::clamp(line_end, 0, screen.size().height);
    line_beg = std::clamp(line_beg, 0, line_end);

    auto const fill_glyph = glyph{
        glyph_style{cursor.style.fg, cursor.style.bg, {}},
        code_point{0}
    };

    std::fill(screen.get_line(line_beg), screen.get_line(line_end), fill_glyph);
    mark_dirty(line_beg, line_end);
}

void terminal::clear(position start, position end)
{
    start = clamp_pos(start);
    end = clamp_pos(end);

    auto const fill_glyph = glyph{
        glyph_style{cursor.style.fg, cursor.style.bg, {}},
        code_point{0}
    };

    std::fill(
        screen.get_line(start.y) + start.x,
        screen.get_line(end.y) + end.x + 1,
        fill_glyph);

    mark_dirty(start.y, end.y);
}

void terminal::delete_chars(int count)
{
    auto cursor_to_end = screen.size().width - cursor.pos.x;
    std::move(
        screen.get_line(cursor.pos.y) + cursor.pos.x + std::min(cursor_to_end, count),
        screen.get_line(cursor.pos.y) + screen.size().width,
        screen.get_line(cursor.pos.y) + cursor.pos.x);

    clear({screen.size().width - 1 - count, cursor.pos.y},
          {screen.size().width - 1, cursor.pos.y});
}

void terminal::insert_blanks(int count)
{
    if (count <= 0)
        return;

    auto const this_line = screen.get_line(cursor.pos.y);
    auto const width = screen.size().width;

    count = std::clamp(count, 1, width - cursor.pos.x);

    std::move_backward(
        this_line + cursor.pos.x,
        this_line + screen.size().width - count,
        this_line + screen.size().width);

    clear(cursor.pos, {cursor.pos.x + count - 1, cursor.pos.y});
}

void terminal::insert_newline(int count)
{
    scroll_down(cursor.pos.y, count);
}

void terminal::dump()
{
    for (int y = 0; y < screen.size().height; ++y) {
        for (int x = 0; x < screen.size().width; ++x) {
            auto pos = position{x, y};
            auto code = screen.get_glyph(pos).code;
            if (code != 0) {
                std::cout << (char)code;
            } else if (cursor.pos == pos) {
                std::cout << '_';
            } else {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }
}

position terminal::clamp_pos(position p)
{
    auto sz = screen.size();

    p.x = p.x < 0         ? 0
        : p.x >= sz.width ? sz.width - 1
        :                   p.x;

    p.y = p.y < 0          ? 0
        : p.y >= sz.height ? sz.height - 1
        :                    p.y;

    return p;
}

} // gd100::
