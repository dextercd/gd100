#include <gd100/terminal_screen.hpp>

terminal_screen::terminal_screen(extend screen_sz)
    : m_size{screen_sz}
    , data{new glyph[m_size.width * m_size.height]{}}
{
}

glyph* terminal_screen::get_line(int line)
{
    return &data[line * m_size.width];
}

glyph& terminal_screen::get_glyph(position pos)
{
    return get_line(pos.y)[pos.x];
}

const glyph* terminal_screen::get_line(int line) const
{
    return &data[line * m_size.width];
}

const glyph& terminal_screen::get_glyph(position pos) const
{
    return get_line(pos.y)[pos.x];
}

extend terminal_screen::size() const
{
    return m_size;
}
