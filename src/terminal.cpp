#include <iostream>
#include <algorithm>

#include <gd100/terminal.hpp>
#include <gd100/terminal_decoder.hpp>

namespace gd100 {

int code_point_width(code_point)
{
    return 1;
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
    auto width{code_point_width(ch)};

    auto* gl{glyph_at_cursor()};
    if (cursor.state.is_set(cursor_state_bit::wrap_next)) {
        gl->style.mode.set(glyph_attr_bit::text_wraps);
        newline(true);
        gl = glyph_at_cursor();
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

void terminal::scroll_up(int keep_top, int down)
{
    int move_to = std::clamp(keep_top, 0, screen.size().height);
    int move_start = std::clamp(move_to + down, 0, screen.size().height);
    int move_end = screen.size().height;

    std::move(
        screen.get_line(move_start),
        screen.get_line(move_end), // pointer past last glyph
        screen.get_line(move_to));

    clear_lines(move_end - down, move_end);

    mark_dirty(move_to, move_end);
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

int terminal::process_bytes(const char* bytes, int length)
{
    auto decoded = decode(bytes, length);
    if (decoded.bytes_consumed == 0)
        return length;

    process_instruction(decoded.instruction);

    auto rest_consumed = process_bytes(
                            bytes + decoded.bytes_consumed,
                            length - decoded.bytes_consumed);

    return decoded.bytes_consumed + rest_consumed;
}

void terminal::process_instruction(terminal_instruction inst)
{
    switch(inst.type) {
        case instruction_type::write_char:
            write_char(inst.write_char.code);
            break;
    }
}

} // gd100::
