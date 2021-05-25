#include <algorithm>
#include <codecvt>
#include <locale>
#include <iostream>
#include <thread>
#include <string>
#include <utility>
#include <mutex>
#include <cstdio>
#include <chrono>

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

godot_variant code_point_key;
godot_variant fg_key;
godot_variant bg_key;

godot_variant get_glyph(gd100::terminal const* const term, int const row, int const column)
{
    godot_dictionary glyph_dict;
    api->godot_dictionary_new(&glyph_dict);

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

    if (glyph.style.mode.is_set(gd100::glyph_attr_bit::reversed))
        std::swap(fg, bg);

    api->godot_dictionary_set(&glyph_dict, &fg_key, &fg);
    api->godot_dictionary_set(&glyph_dict, &bg_key, &bg);

    api->godot_variant_destroy(&bg);
    api->godot_variant_destroy(&fg);

    godot_variant code_point;
    api->godot_variant_new_int(&code_point, glyph.code);

    api->godot_dictionary_set(&glyph_dict, &code_point_key, &code_point);
    api->godot_variant_destroy(&code_point);

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

godot_variant get_lines(gd100::terminal const* const term)
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

godot_variant lines_key;
godot_variant cursor_key;

godot_variant get_terminal_data(const gd100::terminal* const term)
{
    godot_dictionary term_dict;
    api->godot_dictionary_new(&term_dict);

    godot_variant lines_data = get_lines(term);

    api->godot_dictionary_set(&term_dict, &lines_key, &lines_data);
    api->godot_variant_destroy(&lines_data);

    godot_variant cursor_data = get_cursor(term);

    api->godot_dictionary_set(&term_dict, &cursor_key, &cursor_data);
    api->godot_variant_destroy(&cursor_data);

    godot_variant ret;
    api->godot_variant_new_dictionary(&ret, &term_dict);
    api->godot_dictionary_destroy(&term_dict);

    return ret;
}

enum class godot_mouse_button : int {
    none = 0,
    left = 1,
    right = 2,
    middle = 3,
    wheel_up = 4,
    wheel_down = 5,
};

enum class terminal_mouse_button : int {
    none = 0,
    left = 1,
    middle = 2,
    right = 3,
    wheel_up = 4,
    wheel_down = 5,
};

terminal_mouse_button to_terminal_mouse(godot_mouse_button m)
{
    switch(m) {
        case godot_mouse_button::left:       return terminal_mouse_button::left;
        case godot_mouse_button::right:      return terminal_mouse_button::right;
        case godot_mouse_button::middle:     return terminal_mouse_button::middle;
        case godot_mouse_button::wheel_up:   return terminal_mouse_button::wheel_up;
        case godot_mouse_button::wheel_down: return terminal_mouse_button::wheel_down;

        default:
            return terminal_mouse_button::none;
    }
}

bool can_be_held(terminal_mouse_button m)
{
    switch(m) {
        case terminal_mouse_button::left:
        case terminal_mouse_button::middle:
        case terminal_mouse_button::right:
            return true;

        default:
            return false;
    }
}

template<class F>
auto time_call(char const* const label, const F& f)
{
    auto const before = std::chrono::steady_clock::now();
    auto const ret = f();
    auto const after = std::chrono::steady_clock::now();

    auto const diff = std::chrono::duration<double, std::milli>{after - before};

    std::cout << diff.count() << " ms :: " << label << '\n';

    return ret;
}

class terminal_program : public gd100::program {
public:
    gd100::terminal terminal;
    int master_descriptor;
    godot_object* instance;
    gd100::decoder decoder;

    // Mutex necessary to protect access to the terminal and related things.
    //
    // Multi-threaded access can happen when Godot performs some action on the
    // terminal and at the same time data is written to the master_descriptor
    // which is then decoded in handle_bytes.
    //
    // This is not necessary to write/read to the master_descriptor since the
    // kernel guarantees these operations are atomic.
    std::mutex terminal_mutex;

    // -1 so that the first reported mouse position is seen as different.
    int previous_x = -1;
    int previous_y = -1;
    terminal_mouse_button held_button = terminal_mouse_button::none;

    terminal_program(gd100::terminal t, int const md, godot_object* const i)
        : terminal{std::move(t)}
        , master_descriptor{md}
        , instance{i}
    {
    }

    ~terminal_program()
    {
        close(master_descriptor);
    }

    void handle_bytes(const char* bytes, std::size_t const count, bool const more_data_coming) override
    {
        auto lock = std::scoped_lock{terminal_mutex};

        gd100::terminal_instructee t{&terminal};
        time_call("decode", [&] { decoder.decode(bytes, count, t); return 0; });

        if (!more_data_coming) {
            auto data = time_call("serialize-term", [&] { return get_terminal_data(&terminal); });
            const auto* args = &data;
            object_emit_signal_deferred(
                instance,
                api->godot_string_chars_to_utf8("terminal_updated"),
                1,
                &args);

            api->godot_variant_destroy(&data);
        }

#if 0
        std::cerr << "Received " << count << " bytes.\n";
        for(char* b = bytes; b != bytes + count; ++b) {
            std::cerr << ((int)*b) << " ";
        }

        std::cerr << "\n\n\n";
#endif
    }

    void send_code(gd100::code_point const code)
    {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        std::string u8str = converter.to_bytes(code);
        write(master_descriptor, u8str.c_str(), u8str.size());
    }

    void process_mouse(
            int const mouse_x,
            int const mouse_y,
            terminal_mouse_button const button,
            bool const pressed_arg)
    {
        // pressed_arg only makes sense when this is a button event.
        // Some buttons can't be held so they should always be considered pressed.

        // pressed and released will both be false if button is none,
        // if button is not none then only one of them will be true.
        bool const pressed = button != terminal_mouse_button::none
                             && (pressed_arg || !can_be_held(button));

        bool const released = button != terminal_mouse_button::none && !pressed;

        gd100::mouse_mode mode;
        bool is_sgr;

        {
            auto lock = std::scoped_lock{terminal_mutex};
            mode = terminal.mouse;
            is_sgr = terminal.mode.is_set(gd100::terminal_mode_bit::extended_mouse);
        }

        if (mode == gd100::mouse_mode::none) return;
        if (mode == gd100::mouse_mode::x10 && !pressed) return;
        if (mode == gd100::mouse_mode::button && !pressed && !released) return;

        int button_code;

        if (button == terminal_mouse_button::none) { // Motion event
            // Mouse didn't move to different glyhph
            if (mouse_x == previous_x && mouse_y == previous_y)
                return;

            // Motion mode only receives info when a button is held
            if (mode == gd100::mouse_mode::motion
                && held_button == terminal_mouse_button::none)
                return;

            if (held_button == terminal_mouse_button::none)
                button_code = 32 + 3;
            else
                button_code = 32 + static_cast<int>(held_button) - 1;
        } else { // Button event
            if (can_be_held(button)) {
                if (pressed)
                    held_button = button;
                else
                    held_button = terminal_mouse_button::none;
            }

            button_code = static_cast<int>(button) - 1;
            if (button_code >= 3)
                button_code += 64 - 3;
        }

        previous_x = mouse_x;
        previous_y = mouse_y;

        if (!is_sgr) {
            char mouse_data[6]{'\x1b', '[', 'M', /* button, mouse_x, mouse_y */};

            mouse_data[4] = 32 + mouse_x + 1;
            mouse_data[5] = 32 + mouse_y + 1;
            mouse_data[3] = 32 + (released ? 3 : button_code);

            write(master_descriptor, mouse_data, sizeof(mouse_data));
        } else {
            char sgr_buffer[64]{};
            auto const message_length =
                std::snprintf(sgr_buffer, sizeof(sgr_buffer),
                              "\x1b[<%d;%d;%d%c",
                              button_code, mouse_x + 1, mouse_y + 1,
                              released ? 'm' : 'M');
            write(master_descriptor, sgr_buffer, message_length);
        }
    }
};

godot_variant send_code_method(
        godot_object* const obj,
        void* const method_data,
        void* const user_data,
        int const num_args,
        godot_variant** const args)
{
    if (num_args != 1) {
        godot_variant ret;
        api->godot_variant_new_nil(&ret);
        return ret;
    }

    auto const code = api->godot_variant_as_int(args[0]);

    auto term = reinterpret_cast<terminal_program*>(user_data);
    term->send_code(code);

    godot_variant ret;
    api->godot_variant_new_nil(&ret);
    return ret;
}

godot_variant send_mouse_method(
        godot_object* const obj,
        void* const method_data,
        void* const user_data,
        int const num_args,
        godot_variant** const args)
{
    if (num_args != 4) {
        godot_variant ret;
        api->godot_variant_new_nil(&ret);
        return ret;
    }

    auto const mouse_x = api->godot_variant_as_int(args[0]);
    auto const mouse_y = api->godot_variant_as_int(args[1]);
    auto const button_nr = api->godot_variant_as_int(args[2]);
    auto const godot_button = static_cast<godot_mouse_button>(button_nr);
    auto const button = to_terminal_mouse(godot_button);
    auto const pressed = api->godot_variant_as_bool(args[3]);

    auto term = reinterpret_cast<terminal_program*>(user_data);
    term->process_mouse(mouse_x, mouse_y, button, pressed);

    godot_variant ret;
    api->godot_variant_new_nil(&ret);
    return ret;
}

terminal_program* start_program(godot_object* const instance)
{
    auto const masterfd = posix_openpt(O_RDWR | O_NOCTTY);
    grantpt(masterfd);
    unlockpt(masterfd);
    auto const slavename = ptsname(masterfd);
    auto const slavefd = open(slavename, O_RDWR | O_NOCTTY);
    if (slavefd < 0) {
        throw std::runtime_error{"Opening pseudoterminal slave failed."};
    }
    auto const fork_result = fork();

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

        char* const args[]{
            "sh",
            nullptr,
        };

        setenv("TERM", "gdterm", 1);
        execvp("sh", args);
    }

    if (fork_result == -1) {
        abort();
    }

    // parent

    close(slavefd);

    auto const size = gd100::extend{132, 35};


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

    godot_string lines_key_string;
    api->godot_string_new_with_wide_string(&lines_key_string, L"lines", 5);
    api->godot_variant_new_string(&lines_key, &lines_key_string);
    api->godot_string_destroy(&lines_key_string);

    godot_string cursor_key_string;
    api->godot_string_new_with_wide_string(&cursor_key_string, L"cursor", 6);
    api->godot_variant_new_string(&cursor_key, &cursor_key_string);
    api->godot_string_destroy(&cursor_key_string);

    godot_string code_point_key_string;
    api->godot_string_new_with_wide_string(&code_point_key_string, L"code", 4);
    api->godot_variant_new_string(&code_point_key, &code_point_key_string);
    api->godot_string_destroy(&code_point_key_string);

    godot_string fg_key_string;
    api->godot_string_new_with_wide_string(&fg_key_string, L"fg", 2);
    api->godot_variant_new_string(&fg_key, &fg_key_string);
    api->godot_string_destroy(&fg_key_string);

    godot_string bg_key_string;
    api->godot_string_new_with_wide_string(&bg_key_string, L"bg", 2);
    api->godot_variant_new_string(&bg_key, &bg_key_string);
    api->godot_string_destroy(&bg_key_string);
}

void GDTERM_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* options)
{
    api->godot_variant_destroy(&bg_key);
    api->godot_variant_destroy(&fg_key);
    api->godot_variant_destroy(&code_point_key);

    api->godot_variant_destroy(&cursor_key);
    api->godot_variant_destroy(&lines_key);
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

    godot_method_attributes const attr{
        GODOT_METHOD_RPC_MODE_DISABLED
    };

    auto const sc_method = godot_instance_method{
        send_code_method,
        nullptr, nullptr,
    };

    nativescript_api->godot_nativescript_register_method(
        desc,
        "TerminalLogic",
        "send_code",
        attr,
        sc_method);

    auto const sm_method = godot_instance_method{
        send_mouse_method,
        nullptr, nullptr,
    };

    nativescript_api->godot_nativescript_register_method(
        desc,
        "TerminalLogic",
        "send_mouse",
        attr,
        sm_method);
}

}
