#include <algorithm>

#include <gdl/api.hpp>

namespace gdl {

godot_gdnative_api_struct const* find_extension_of_type(unsigned const type)
{
    auto const extensions_begin = api->extensions;
    auto const extensions_end = extensions_begin + api->num_extensions;

    auto const ext = std::find_if(extensions_begin, extensions_end,
                        [=](auto const extension) {
                            return extension->type == type;
                        });

    if (ext != extensions_end)
        return *ext;

    return nullptr;
}

void initialise(godot_gdnative_init_options* options)
{
    api = options->api_struct;
    nativescript_api = reinterpret_cast<decltype(nativescript_api)>(
                            find_extension_of_type(GDNATIVE_EXT_NATIVESCRIPT));
}

void deinitialise()
{
    nativescript_api = nullptr;
    api = nullptr;
}

} // gdl::
