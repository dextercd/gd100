#ifndef GDL_DICTIONARY_HPP
#define GDL_DICTIONARY_HPP

#include "api.hpp"
#include "lifetime.hpp"
#include "variant.hpp"

namespace gdl {

template<>
struct native_handle_funcs<godot_dictionary> {
    static godot_dictionary new_default()
    {
        godot_dictionary ret;
        api->godot_dictionary_new(&ret);
        return ret;
    }

    static godot_dictionary new_copy(godot_dictionary dictionary)
    {
        godot_dictionary ret;
        api->godot_dictionary_new_copy(&ret, &dictionary);
        return ret;
    }

    static void destroy(godot_dictionary dictionary)
    {
        api->godot_dictionary_destroy(&dictionary);
    }
};

class dictionary : public lifetime<godot_dictionary>
{
public:
    void set(variant const& key, variant const& value)
    {
        api->godot_dictionary_set(&m_native_handle, key.get(), value.get());
    }
};

inline godot_variant to_variant_handle(dictionary const& d)
{
    godot_variant ret;
    api->godot_variant_new_dictionary(&ret, d.get());
    return ret;
}

} // gdl::

#endif // header guard
