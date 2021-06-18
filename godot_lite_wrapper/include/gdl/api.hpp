#ifndef GDL_API_HPP
#define GDL_API_HPP

#include <gdnative_api_struct.gen.h>

namespace gdl {

inline godot_gdnative_core_api_struct const* api = nullptr;
inline godot_gdnative_ext_nativescript_api_struct const* nativescript_api = nullptr;

void initialise(godot_gdnative_init_options* options);
void deinitialise();

} // gdl::

#endif // header guard
