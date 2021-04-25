#include <iostream>

#include <gd100/position.hpp>

namespace gd100 {

std::ostream& operator<<(std::ostream& os, position const p)
{
    return os << '(' << p.x << "; " << p.y << ')';
}

} // gd100::
