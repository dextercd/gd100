#ifndef GDTERM_TERMINAL_SCREEN_HPP
#define GDTERM_TERMINAL_SCREEN_HPP

#include <memory>

#include "glyph.hpp"

class terminal_screen {
public:
    extend size;

private:
    std::unique_ptr<glyph[]> data;

public:
    terminal_screen()
        : terminal_screen({80, 25})
    {
    }

    terminal_screen(extend screen_sz);

    glyph* get_line(int line);
    glyph& get_glyph(position pos);

    glyph const* get_line(int line) const;
    glyph const& get_glyph(position pos) const;
};

#endif // header guard
