#include <gdnative_api_struct.gen.h>

#include "gdterm_export.h"

namespace {

godot_gdnative_core_api_struct const* api = nullptr;

}

extern "C" {

void GDTERM_EXPORT godot_gdnative_init(godot_gdnative_init_options* options)
{
    api = options->api_struct;
}

void GDTERM_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* options)
{
}

}
