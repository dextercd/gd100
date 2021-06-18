#ifndef GDTERM_BIT_CONTAINER_HPP
#define GDTERM_BIT_CONTAINER_HPP

#include <type_traits>

namespace gd100 {

template<class bit_type>
class bit_container
{
    using data_type = std::underlying_type_t<bit_type>;
    data_type data{};

public:
    bit_container() = default;
    bit_container(bit_type bit)
    {
        data = static_cast<data_type>(bit);
    }

    void set(bit_container bits)
    {
        data |= bits.data;
    }

    bool is_set(bit_container bits) const
    {
        return (data & bits.data) == bits.data;
    }

    void unset(bit_container bits)
    {
        data &= ~bits.data;
    }

    friend bool operator==(bit_container left, bit_container right)
    {
        return left.data == right.data;
    }

    friend bool operator!=(bit_container left, bit_container right)
    {
        return !(left == right);
    }
};

} // gd100::

#endif // header guard
