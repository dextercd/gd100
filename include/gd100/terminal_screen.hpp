#ifndef GDTERM_TERMINAL_SCREEN_HPP
#define GDTERM_TERMINAL_SCREEN_HPP

#include <vector>
#include <memory>

#include "glyph.hpp"

namespace gd100 {

class terminal_screen {
private:
    extend m_size;

public:
    std::vector<std::unique_ptr<glyph[]>> lines;

    terminal_screen()
        : terminal_screen({80, 25})
    {
    }

    terminal_screen(extend screen_sz);

    glyph* get_line(int line);
    glyph& get_glyph(position pos);

    glyph const* get_line(int line) const;
    glyph const& get_glyph(position pos) const;

    extend size() const;
};

} // gd100::

#endif // header guard
