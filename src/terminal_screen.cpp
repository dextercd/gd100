#include <type_traits>

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
        line = data.data() + index * m_size.width;
        ++index;
    }
}

glyph* terminal_screen::get_line(int line)
{
    return lines[line];
}

glyph& terminal_screen::get_glyph(position pos)
{
    return get_line(pos.y)[pos.x];
}

const glyph* terminal_screen::get_line(int line) const
{
    return lines[line];
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
