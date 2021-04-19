#include "terminal_screen.hpp"

terminal_screen::terminal_screen(extend screen_sz)
    : size{screen_sz}
    , data{new glyph[size.width * size.height]}
{
}

glyph* terminal_screen::get_line(int line)
{
    return &data[line * size.width];
}

glyph& terminal_screen::get_glyph(position pos)
{
    return get_line(pos.y)[pos.x];
}
