#include <algorithm>
#include <codecvt>
#include <locale>
#include <iostream>
#include <thread>
#include <string>

#include <gdnative_api_struct.gen.h>

#include "gdterm_export.h"
#include <gd100/terminal.hpp>
#include <gd100/program.hpp>
#include <gd100/program_terminal_manager.hpp>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

gd100::program_terminal_manager manager;

namespace {

godot_gdnative_core_api_struct const* api = nullptr;
godot_gdnative_ext_nativescript_api_struct const* nativescript_api = nullptr;

}

godot_variant_call_error object_emit_signal_deferred(
        godot_object *p_object, godot_string p_signal_name,
        int p_num_args, const godot_variant **p_args)
{
    godot_variant variant;
    api->godot_variant_new_object(&variant, p_object);

    godot_string method = api->godot_string_chars_to_utf8("call_deferred");

    const godot_variant** args = (const godot_variant**)api->godot_alloc(sizeof(godot_variant *) * (p_num_args + 2));

    godot_variant variant_method_name;
    godot_string method_name = api->godot_string_chars_to_utf8("emit_signal");
    api->godot_variant_new_string(&variant_method_name, &method_name);
    api->godot_string_destroy(&method_name);

    godot_variant variant_signal_name;
    api->godot_variant_new_string(&variant_signal_name, &p_signal_name);

    args[0] = &variant_method_name;
    args[1] = &variant_signal_name;

    for (int i = 0; i < p_num_args; i++)
    {
        args[i + 2] = p_args[i];
    }

    godot_variant_call_error error;
    api->godot_variant_call(&variant, &method, args, p_num_args + 2, &error);

    api->godot_variant_destroy(&variant_signal_name);
    api->godot_string_destroy(&method_name);
    api->godot_variant_destroy(&variant_method_name);
    api->godot_free(args);
    api->godot_string_destroy(&method);
    api->godot_variant_destroy(&variant);

    return error;
}

godot_variant get_lines(gd100::terminal* term)
{
    godot_dictionary topdict;
    api->godot_dictionary_new(&topdict);

    for (int line = 0; line != term->screen.size().height; ++line) {
        godot_array linearr;
        api->godot_array_new(&linearr);

        for(int i = 0; i != term->screen.size().width; ++i) {
            godot_dictionary glyph_data;
            api->godot_dictionary_new(&glyph_data);

            godot_string code_point_key;
            api->godot_string_new_with_wide_string(&code_point_key, L"code", 4);
            godot_variant code_point_key_var;
            api->godot_variant_new_string(&code_point_key_var, &code_point_key);

            godot_variant gcode;
            api->godot_variant_new_int(&gcode, term->screen.get_glyph({i, line}).code);

            api->godot_dictionary_set(&glyph_data, &code_point_key_var, &gcode);

            godot_variant glyph_var;
            api->godot_variant_new_dictionary(&glyph_var, &glyph_data);

            api->godot_array_push_back(&linearr, &glyph_var);
        }

        godot_variant gline;
        api->godot_variant_new_int(&gline, line);

        godot_variant linevar;
        api->godot_variant_new_array(&linevar, &linearr);

        api->godot_dictionary_set(&topdict, &gline, &linevar);
    }

    godot_variant ret;
    api->godot_variant_new_dictionary(&ret, &topdict);
    api->godot_dictionary_destroy(&topdict);

    return ret;
}

class terminal_program : public gd100::program {
public:
    gd100::terminal terminal;
    int master_descriptor;
    godot_object* instance;
    std::string write_buffer;

    terminal_program(gd100::terminal t, int md, godot_object* i)
        : terminal{std::move(t)}
        , master_descriptor{md}
        , instance{i}
    {
    }

    void handle_bytes(char* bytes, std::size_t count) override
    {
        write_buffer.append(bytes, count);
        auto processed = terminal.process_bytes(write_buffer.c_str(), write_buffer.size());
        write_buffer.erase(0, processed);

        auto lines = get_lines(&terminal);
        const auto* args = &lines;
        object_emit_signal_deferred(
            instance,
            api->godot_string_chars_to_utf8("terminal_updated"),
            1,
            &args);

        std::cerr << "Received " << count << " bytes.\n";
        for(char* b = bytes; b != bytes + count; ++b)
            std::cerr << ((int)*b) << ' ';
        std::cerr << "----\n";
    }

    void send_code(std::uint64_t code)
    {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        std::string u8str = converter.to_bytes(code);
        write(master_descriptor, u8str.c_str(), u8str.size());
        std::cerr << code << '\n';
    }
};

godot_variant send_code_method(
        godot_object *obj,
        void *method_data,
        void *user_data,
        int num_args,
        godot_variant **args)
{
    if (num_args != 1) {
        godot_variant ret;
        api->godot_variant_new_nil(&ret);
        return ret;
    }

    auto code = api->godot_variant_as_int(args[0]);

    auto term = reinterpret_cast<terminal_program*>(user_data);
    term->send_code(code);

    godot_variant ret;
    api->godot_variant_new_nil(&ret);
    return ret;
}

terminal_program* start_program(godot_object* const instance)
{
    auto masterfd = posix_openpt(O_RDWR | O_NOCTTY);
    grantpt(masterfd);
    unlockpt(masterfd);
    auto slavename = ptsname(masterfd);
    auto slavefd = open(slavename, O_RDWR | O_NOCTTY);
    auto fork_result = fork();

    // child
    if (fork_result == 0) {
        close(masterfd);

        setsid();
        ioctl(slavefd, TIOCSCTTY, 0);

        close(0);
        close(1);
        close(2);
        dup2(slavefd, 0);
        dup2(slavefd, 1);
        dup2(slavefd, 2);
        execl("/bin/sh", "sh", (char*)nullptr);
    }

    if (fork_result == -1) {
        abort();
    }

    // parent

    close(slavefd);

    auto const size = gd100::extend{90, 30};


    auto const winsz = winsize{
        static_cast<unsigned short>(size.height),
        static_cast<unsigned short>(size.width),
        0, 0
    };

    if(ioctl(masterfd, TIOCSWINSZ, &winsz))
        throw std::runtime_error{"Couldn't set window size."};

    auto program = std::make_unique<terminal_program>(
        gd100::terminal{size},
        masterfd,
        instance
    );

    return (terminal_program*)manager.register_program(masterfd, std::move(program));
}

void* create_terminal(godot_object* const instance, void* const method_data)
{
    return start_program(instance);
}

void destroy_terminal(godot_object* const instance, void* const method_data, void* user_data)
{
    auto term = reinterpret_cast<terminal_program*>(user_data);
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

    godot_variant nil;
    api->godot_variant_new_nil(&nil);

    auto signal_arg = godot_signal_argument{
        api->godot_string_chars_to_utf8("data"),
        GODOT_VARIANT_TYPE_DICTIONARY,
        GODOT_PROPERTY_HINT_NONE,
        api->godot_string_chars_to_utf8("hint str"),
        GODOT_PROPERTY_USAGE_DEFAULT,
        nil
    };

    auto const signal = godot_signal{
        api->godot_string_chars_to_utf8("terminal_updated"),
        1, &signal_arg,
        0, nullptr,
    };

    nativescript_api->godot_nativescript_register_signal(
        desc,
        "TerminalLogic",
        &signal);

    godot_method_attributes attr{
        GODOT_METHOD_RPC_MODE_DISABLED
    };

    auto method = godot_instance_method{
        send_code_method,
        nullptr, nullptr,
    };

    nativescript_api->godot_nativescript_register_method(
        desc,
        "TerminalLogic",
        "send_code",
        attr,
        method);
}

void GDTERM_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* options)
{
}

}
