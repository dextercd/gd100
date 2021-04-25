#include <iostream>

#include <gd100/position.hpp>

std::ostream& operator<<(std::ostream& os, position const p)
{
    return os << '(' << p.x << "; " << p.y << ')';
}
