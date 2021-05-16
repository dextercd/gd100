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

godot_variant get_glyph(gd100::terminal const* const term, int const row, int const column)
{
    godot_dictionary glyph_dict;
    api->godot_dictionary_new(&glyph_dict);

    godot_variant code_point_key;
    {
        godot_string code_point_key_string;
        api->godot_string_new_with_wide_string(&code_point_key_string, L"code", 4);
        api->godot_variant_new_string(&code_point_key, &code_point_key_string);
        api->godot_string_destroy(&code_point_key_string);
    }

    godot_variant fg_key;
    {
        godot_string fg_key_string;
        api->godot_string_new_with_wide_string(&fg_key_string, L"fg", 2);
        api->godot_variant_new_string(&fg_key, &fg_key_string);
        api->godot_string_destroy(&fg_key_string);
    }

    godot_variant bg_key;
    {
        godot_string bg_key_string;
        api->godot_string_new_with_wide_string(&bg_key_string, L"bg", 2);
        api->godot_variant_new_string(&bg_key, &bg_key_string);
        api->godot_string_destroy(&bg_key_string);
    }

    auto glyph = term->screen.get_glyph({column, row});

    godot_color fg_col;
    api->godot_color_new_rgb(&fg_col,
        glyph.style.fg.r / 255.0,
        glyph.style.fg.g / 255.0,
        glyph.style.fg.b / 255.0);

    godot_variant fg;
    api->godot_variant_new_color(&fg, &fg_col);

    godot_color bg_col;
    api->godot_color_new_rgb(&bg_col,
        glyph.style.bg.r / 255.0,
        glyph.style.bg.g / 255.0,
        glyph.style.bg.b / 255.0);

    godot_variant bg;
    api->godot_variant_new_color(&bg, &bg_col);

    api->godot_dictionary_set(&glyph_dict, &fg_key, &fg);
    api->godot_dictionary_set(&glyph_dict, &bg_key, &bg);

    api->godot_variant_destroy(&bg);
    api->godot_variant_destroy(&fg);

    api->godot_variant_destroy(&bg_key);
    api->godot_variant_destroy(&fg_key);

    godot_variant code_point;
    api->godot_variant_new_int(&code_point, glyph.code);

    api->godot_dictionary_set(&glyph_dict, &code_point_key, &code_point);
    api->godot_variant_destroy(&code_point);
    api->godot_variant_destroy(&code_point_key);

    godot_variant ret;
    api->godot_variant_new_dictionary(&ret, &glyph_dict);
    api->godot_dictionary_destroy(&glyph_dict);

    return ret;
}

godot_variant get_line(gd100::terminal const* const term, int const line)
{
    godot_array linearr;
    api->godot_array_new(&linearr);

    auto const width = term->screen.size().width;

    for(int col = 0; col != width; ++col) {
        auto glyph = get_glyph(term, line, col);
        api->godot_array_push_back(&linearr, &glyph);
        api->godot_variant_destroy(&glyph);
    }

    godot_variant ret;
    api->godot_variant_new_array(&ret, &linearr);
    api->godot_array_destroy(&linearr);

    return ret;
}

godot_variant get_lines(gd100::terminal* term)
{
    godot_dictionary line_dict;
    api->godot_dictionary_new(&line_dict);

    for (int line = 0; line != term->screen.size().height; ++line) {

        godot_variant line_number;
        api->godot_variant_new_int(&line_number, line);

        godot_variant line_data = get_line(term ,line);

        api->godot_dictionary_set(&line_dict, &line_number, &line_data);

        api->godot_variant_destroy(&line_number);
        api->godot_variant_destroy(&line_data);
    }

    godot_variant ret;
    api->godot_variant_new_dictionary(&ret, &line_dict);
    api->godot_dictionary_destroy(&line_dict);

    return ret;
}

godot_variant get_cursor(gd100::terminal const* const term)
{
    godot_vector2 cursor_pos;
    api->godot_vector2_new(&cursor_pos, term->cursor.pos.x, term->cursor.pos.y);

    godot_variant ret;
    api->godot_variant_new_vector2(&ret, &cursor_pos);

    return ret;
}

godot_variant get_terminal_data(gd100::terminal* term)
{
    godot_dictionary term_dict;
    api->godot_dictionary_new(&term_dict);

    godot_variant lines_key;
    {
        godot_string lines_key_string;
        api->godot_string_new_with_wide_string(&lines_key_string, L"lines", 5);
        api->godot_variant_new_string(&lines_key, &lines_key_string);
        api->godot_string_destroy(&lines_key_string);
    }

    godot_variant lines_data = get_lines(term);

    api->godot_dictionary_set(&term_dict, &lines_key, &lines_data);
    api->godot_variant_destroy(&lines_data);
    api->godot_variant_destroy(&lines_key);

    godot_variant cursor_key;
    {
        godot_string cursor_key_string;
        api->godot_string_new_with_wide_string(&cursor_key_string, L"cursor", 6);
        api->godot_variant_new_string(&cursor_key, &cursor_key_string);
        api->godot_string_destroy(&cursor_key_string);
    }

    godot_variant cursor_data = get_cursor(term);

    api->godot_dictionary_set(&term_dict, &cursor_key, &cursor_data);
    api->godot_variant_destroy(&cursor_data);
    api->godot_variant_destroy(&cursor_key);

    godot_variant ret;
    api->godot_variant_new_dictionary(&ret, &term_dict);
    api->godot_dictionary_destroy(&term_dict);

    return ret;
}

class terminal_program : public gd100::program {
public:
    gd100::terminal terminal;
    int master_descriptor;
    godot_object* instance;
    gd100::decoder decoder;

    terminal_program(gd100::terminal t, int md, godot_object* i)
        : terminal{std::move(t)}
        , master_descriptor{md}
        , instance{i}
    {
    }

    ~terminal_program()
    {
        close(master_descriptor);
    }

    void handle_bytes(char* bytes, std::size_t count) override
    {
        gd100::terminal_instructee t{&terminal};
        decoder.decode(bytes, count, t);

        auto data = get_terminal_data(&terminal);
        const auto* args = &data;
        object_emit_signal_deferred(
            instance,
            api->godot_string_chars_to_utf8("terminal_updated"),
            1,
            &args);

        api->godot_variant_destroy(&data);

#if 0
        std::cerr << "Received " << count << " bytes.\n";
        for(char* b = bytes; b != bytes + count; ++b) {
            std::cerr << ((int)*b) << " ";
        }

        std::cerr << "\n\n\n";
#endif
    }

    void send_code(std::uint64_t code)
    {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        std::string u8str = converter.to_bytes(code);
        write(master_descriptor, u8str.c_str(), u8str.size());
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
    if (slavefd < 0) {
        throw std::runtime_error{"Opening pseudoterminal slave failed."};
    }
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

    auto const size = gd100::extend{132, 32};


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
