#include <iostream>
#include <algorithm>

#include <gd100/terminal.hpp>
#include <gd100/terminal_decoder.hpp>
#include <cw/character_width.hpp>

namespace gd100 {

static_assert(std::is_copy_assignable_v<terminal>);

charset terminal::current_charset() const
{
    return translation_tables[using_translation_table];
}

void terminal::tab()
{
    auto constexpr tab_size = 8;
    auto const new_x = cursor.pos.x + tab_size - ((cursor.pos.x + tab_size) % tab_size);
    move_cursor({new_x, cursor.pos.y});
}

void terminal::newline(bool const first_column)
{
    if (cursor.pos.y + 1 < screen.size().height) {
        cursor.pos.y++;
    } else {
        scroll_up(0, 1);
    }

    if (first_column)
        cursor.pos.x = 0;
}

void terminal::write_char(code_point const ch)
{
    auto const width{cw::character_width(ch)};
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

    set_char(ch, width, cursor.style, cursor.pos);

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

void terminal::move_cursor(position const pos)
{
    cursor.pos = clamp_pos(pos);
    cursor.state.unset(cursor_state_bit::wrap_next);
}

void terminal::move_cursor_forward(int const width)
{
    move_cursor({cursor.pos.x + width, cursor.pos.y});
}

void terminal::set_char(
        code_point ch,
        int const width,
        glyph_style const style,
        position pos)
{
    pos = clamp_pos(pos);

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

    if (width == 2) {
        screen.get_glyph(pos).style.mode.set(glyph_attr_bit::wide);

        if (pos.x + 1 < screen.size().width) {
            auto const dummy_pos = position{pos.x + 1, pos.y};
            screen.get_glyph(dummy_pos) = {style, '\0'};
            screen.get_glyph(dummy_pos).style.mode = glyph_attr_bit::wdummy;
        }
    }
}

glyph* terminal::glyph_at_cursor()
{
    return &screen.get_glyph(cursor.pos);
}

void terminal::mark_dirty(int const line)
{
    mark_dirty(line, line + 1);
}

void terminal::mark_dirty(int const line_beg, int const line_end)
{
    screen.mark_dirty(line_beg, line_end);
}

void terminal::scroll_up(int keep_top, int const count)
{
    auto const height = screen.size().height;

    keep_top = std::clamp(keep_top, 0, height);
    auto const move_end = height;
    auto const move_to = std::clamp(keep_top + count, 0, height);

    std::rotate(
        screen.lines.begin() + keep_top,
        screen.lines.begin() + move_to,
        screen.lines.begin() + move_end);

    clear_lines(move_end - count, move_end);

    if (keep_top < height / 2) {
        mark_dirty(0, keep_top);
        screen.move_scroll(count);
    } else {
        screen.mark_dirty(keep_top, move_end);
    }
}

void terminal::scroll_down(int keep_top, int const count)
{
    auto const height = screen.size().height;

    keep_top = std::clamp(keep_top, 0, height);
    auto const move_end = height;
    auto const move_to = std::clamp(move_end - count, keep_top, height);

    std::rotate(
        screen.lines.begin() + keep_top,
        screen.lines.begin() + move_to,
        screen.lines.begin() + move_end);

    clear_lines(move_end - count, move_end);

    if (keep_top < height / 2) {
        mark_dirty(0, keep_top);
        screen.move_scroll(-count);
    } else {
        screen.mark_dirty(keep_top, move_end);
    }
}

void terminal::clear_lines(int line_beg, int line_end)
{
    line_end = std::clamp(line_end, 0, screen.size().height);
    line_beg = std::clamp(line_beg, 0, line_end);

    auto const fill_glyph = glyph{
        glyph_style{cursor.style.fg, cursor.style.bg, {}},
        code_point{0}
    };

    for(auto line_it{line_beg}; line_it != line_end; ++line_it) {
        std::fill(
            screen.get_line(line_it),
            screen.get_line(line_it) + screen.size().width,
            fill_glyph);
    }

    mark_dirty(line_beg, line_end);
}

void terminal::clear(position start, position end)
{
    start = clamp_pos(start);
    end = clamp_pos(end);

    if ((start.y > end.y) || (start.y == end.y && start.x > end.x))
        return;

    auto const fill_glyph = glyph{
        glyph_style{cursor.style.fg, cursor.style.bg, {}},
        code_point{0}
    };

    auto it = start;
    while(it != end) {
        screen.get_glyph(it) = fill_glyph;
        ++it.x;
        if (it.x == screen.size().width) {
            it.x = 0;
            ++it.y;
        }
    }
    screen.get_glyph(end) = fill_glyph;

    mark_dirty(start.y, end.y + 1);
}

void terminal::delete_chars(int count)
{
    auto const cursor_to_end = screen.size().width - cursor.pos.x;
    std::move(
        screen.get_line(cursor.pos.y) + cursor.pos.x + std::min(cursor_to_end, count),
        screen.get_line(cursor.pos.y) + screen.size().width,
        screen.get_line(cursor.pos.y) + cursor.pos.x);

    clear({screen.size().width - count, cursor.pos.y},
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

void terminal::insert_newline(int const count)
{
    scroll_down(cursor.pos.y, count);
}

void terminal::dump()
{
    for (int y = 0; y < screen.size().height; ++y) {
        for (int x = 0; x < screen.size().width; ++x) {
            auto const pos = position{x, y};
            auto const code = screen.get_glyph(pos).code;
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
    auto const sz = screen.size();

    p.x = p.x < 0         ? 0
        : p.x >= sz.width ? sz.width - 1
        :                   p.x;

    p.y = p.y < 0          ? 0
        : p.y >= sz.height ? sz.height - 1
        :                    p.y;

    return p;
}

void terminal::reset_style()
{
    cursor.style = default_style;
}

} // gd100::
