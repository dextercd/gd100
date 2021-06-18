#ifndef GDL_STRING_HPP
#define GDL_STRING_HPP

#include "api.hpp"
#include "lifetime.hpp"
#include "variant.hpp"

namespace gdl {

template<>
struct native_handle_funcs<godot_string> {
    static godot_string new_default()
    {
        godot_string ret;
        api->godot_string_new(&ret);
        return ret;
    }

    static godot_string new_copy(godot_string string)
    {
        godot_string ret;
        api->godot_string_new_copy(&ret, &string);
        return ret;
    }

    static void destroy(godot_string string)
    {
        api->godot_string_destroy(&string);
    }
};

struct bad_utf8 : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class string : public lifetime<godot_string>
{
    static godot_string from_wide_string(
            wchar_t const* const content,
            int const size)
    {
        godot_string ret;
        api->godot_string_new_with_wide_string(&ret, content, size);
        return ret;
    }

public:
    string() = default;
    string(wchar_t const* const content, int const size)
        : lifetime{from_wide_string(content, size)}
    {
    }

    string(char const* const content, int const size)
        : lifetime{api->godot_string_chars_to_utf8_with_len(content, size)}
    {
    }

    string(char const* const content)
        : lifetime{api->godot_string_chars_to_utf8(content)}
    {
    }
};

inline godot_variant to_variant_handle(string const& d)
{
    godot_variant ret;
    api->godot_variant_new_string(&ret, d.get());
    return ret;
}

} // gdl::

#endif // header guard
