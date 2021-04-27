#ifndef GDTERM_PROGRAM_HPP
#define GDTERM_PROGRAM_HPP

#include <cstdint>

namespace gd100 {

class program {
public:
    virtual void handle_bytes(char*, std::size_t) = 0;
    virtual ~program() = default;
};

} // gd100::

#endif // header guard
