#ifndef GDL_VARIANT_HPP
#define GDL_VARIANT_HPP

#include <cstdint>
#include <utility>

#include "api.hpp"
#include "lifetime.hpp"

namespace gdl {

template<>
struct native_handle_funcs<godot_variant> {
    static godot_variant new_default()
    {
        godot_variant ret;
        api->godot_variant_new_nil(&ret);
        return ret;
    }

    static godot_variant new_copy(godot_variant array)
    {
        godot_variant ret;
        api->godot_variant_new_copy(&ret, &array);
        return ret;
    }

    static void destroy(godot_variant array)
    {
        api->godot_variant_destroy(&array);
    }
};

inline godot_variant to_variant_handle(std::uint64_t v)
{
    godot_variant ret;
    api->godot_variant_new_uint(&ret, v);
    return ret;
}

inline godot_variant to_variant_handle(std::int64_t v)
{
    godot_variant ret;
    api->godot_variant_new_int(&ret, v);
    return ret;
}

template<class T>
concept fits_in_variant = requires(T a) {
    { to_variant_handle(std::move(a)) } -> std::convertible_to<godot_variant>;
};

class variant : public lifetime<godot_variant> {
public:
    variant() = default;

    template<fits_in_variant F>
    variant(F&& item)
        : lifetime{to_variant_handle(std::forward<F>(item))}
    {
    }

    variant(godot_variant v)
        : lifetime(v)
    {
    }
};

}

#endif // header guard
