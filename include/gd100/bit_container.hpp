#ifndef GDTERM_BIT_CONTAINER_HPP
#define GDTERM_BIT_CONTAINER_HPP

#include <type_traits>

template<class bit_type>
class bit_container
{
    using data_type = std::underlying_type_t<bit_type>;
    data_type data{};

public:
    bit_container() = default;
    explicit bit_container(bit_type bit)
    {
        set(bit);
    }

    void set(bit_type bit)
    {
        data |= static_cast<data_type>(bit);
    }

    bool is_set(bit_type bit)
    {
        return data & static_cast<data_type>(bit);
    }

    void unset(bit_type bit)
    {
        data &= ~static_cast<data_type>(bit);
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

#endif // header guard
