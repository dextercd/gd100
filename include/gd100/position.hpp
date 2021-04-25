#ifndef GDTERM_POSITION_HPP
#define GDTERM_POSITION_HPP

#include <iosfwd>

namespace gd100 {

struct position {
    int x;
    int y;

    friend bool operator==(position left, position right)
    {
        return left.x == right.x && left.y == right.y;
    }

    friend bool operator!=(position left, position right)
    {
        return !(left == right);
    }
};

std::ostream& operator<<(std::ostream&, position);

} // gd100::

#endif // header guard
