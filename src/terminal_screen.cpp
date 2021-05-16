#include <gd100/terminal_screen.hpp>

namespace gd100 {

terminal_screen::terminal_screen(extend screen_sz)
    : m_size{screen_sz}
{
    lines.resize(m_size.height);
    for(auto& line : lines)
        line.reset(new glyph[m_size.width]{});
}

glyph* terminal_screen::get_line(int line)
{
    return lines[line].get();
}

glyph& terminal_screen::get_glyph(position pos)
{
    return get_line(pos.y)[pos.x];
}

const glyph* terminal_screen::get_line(int line) const
{
    return lines[line].get();
}

const glyph& terminal_screen::get_glyph(position pos) const
{
    return get_line(pos.y)[pos.x];
}

extend terminal_screen::size() const
{
    return m_size;
}

} // gd100::
