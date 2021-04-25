#include <algorithm>
#include <iostream>

#include <gdnative_api_struct.gen.h>

#include "gdterm_export.h"
#include <gd100/terminal.hpp>

namespace {

godot_gdnative_core_api_struct const* api = nullptr;
godot_gdnative_ext_nativescript_api_struct const* nativescript_api = nullptr;

}

void* create_terminal(godot_object* const instance, void* const method_data)
{
    auto term = std::make_unique<terminal>();
    return term.release();
}

void destroy_terminal(godot_object* const instance, void* const method_data, void* user_data)
{
    auto term = std::unique_ptr<terminal>{(terminal*)user_data};
}

/* Looks in api->extensions for an extension that matches the argument type.
   Returns nullptr when an extension of that type could not be found. */
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

extern "C" {

void GDTERM_EXPORT godot_gdnative_init(godot_gdnative_init_options* options)
{
    api = options->api_struct;
    nativescript_api = reinterpret_cast<decltype(nativescript_api)>(
                            find_extension_of_type(GDNATIVE_EXT_NATIVESCRIPT));
}

void GDTERM_EXPORT godot_nativescript_init(void* desc)
{
    auto const create = godot_instance_create_func{create_terminal, nullptr, nullptr};
    auto const destroy = godot_instance_destroy_func{destroy_terminal, nullptr, nullptr};

    nativescript_api->godot_nativescript_register_class(
        desc,
        "TerminalLogic",
        "Reference",
        create, destroy);
}

void GDTERM_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* options)
{
}

}
