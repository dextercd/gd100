#include <type_traits>
#include <algorithm>

#include <gd100/terminal_screen.hpp>

namespace gd100 {

static_assert(std::is_copy_assignable_v<terminal_screen>);

terminal_screen::terminal_screen(extend screen_sz)
    : m_size{screen_sz}
    , data(m_size.width * m_size.height, glyph{})
{
    lines.resize(m_size.height);
    int index = 0;
    for(auto& line : lines) {
        line.glyphs = data.data() + index * m_size.width;
        line.changed = false;
        ++index;
    }
}

glyph* terminal_screen::get_line(int line)
{
    return lines[line].glyphs;
}

glyph& terminal_screen::get_glyph(position pos)
{
    return get_line(pos.y)[pos.x];
}

const glyph* terminal_screen::get_line(int line) const
{
    return lines[line].glyphs;
}

const glyph& terminal_screen::get_glyph(position pos) const
{
    return get_line(pos.y)[pos.x];
}

extend terminal_screen::size() const
{
    return m_size;
}

void terminal_screen::mark_dirty(int start, int end)
{
    auto const height = size().height;
    start = std::clamp(start, 0, height);
    end = std::clamp(end, 0, height);

    while(start < end)
        lines[start++].changed = true;
}

int terminal_screen::changed_scroll() const
{
    return m_scroll;
}

void terminal_screen::clear_changes()
{
    set_scroll(0);
    for (auto& line : lines)
        line.changed = false;
}

void terminal_screen::move_scroll(int change)
{
    set_scroll(m_scroll + change);
}

void terminal_screen::set_scroll(int const scroll)
{
    auto const height = size().height;
    auto const bounded = scroll % height;
    if (bounded >= 0)
        m_scroll = bounded;
    else
        m_scroll = height + bounded;
}

} // gd100::
