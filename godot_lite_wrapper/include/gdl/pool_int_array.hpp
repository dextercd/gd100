#ifndef GDL_POOL_INT_ARRAY_HPP
#define GDL_POOL_INT_ARRAY_HPP

#include "api.hpp"
#include "lifetime.hpp"

namespace gdl {

template<>
struct native_handle_funcs<godot_pool_int_array> {
    static godot_pool_int_array new_default()
    {
        godot_pool_int_array ret;
        api->godot_pool_int_array_new(&ret);
        return ret;
    }

    static godot_pool_int_array new_copy(godot_pool_int_array array)
    {
        godot_pool_int_array ret;
        api->godot_pool_int_array_new_copy(&ret, &array);
        return ret;
    }

    static void destroy(godot_pool_int_array array)
    {
        api->godot_pool_int_array_destroy(&array);
    }
};

class pool_int_array : public lifetime<godot_pool_int_array>
{
public:
    void resize(int size)
    {
        api->godot_pool_int_array_resize(&m_native_handle, size);
    }

    void set(int index, int value)
    {
        api->godot_pool_int_array_set(&m_native_handle, index, value);
    }
};

godot_variant to_variant_handle(pool_int_array const& arr)
{
    godot_variant ret;
    api->godot_variant_new_pool_int_array(&ret, arr.get());
    return ret;
}

} // gdl::

#endif // header guard
