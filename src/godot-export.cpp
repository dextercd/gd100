#include <algorithm>
#include <chrono>
#include <codecvt>
#include <cstdio>
#include <iostream>
#include <locale>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

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

#include <gdl/api.hpp>
#include <gdl/pool_int_array.hpp>
#include <gdl/variant.hpp>
#include <gdl/dictionary.hpp>
#include <gdl/string.hpp>

gd100::program_terminal_manager manager;

godot_variant_call_error object_emit_signal_deferred(
        godot_object *p_object, gdl::string const& p_signal_name,
        int p_num_args, const godot_variant **p_args)
{
    godot_variant variant;
    gdl::api->godot_variant_new_object(&variant, p_object);

    const godot_variant** args = (const godot_variant**)gdl::api->godot_alloc(sizeof(godot_variant *) * (p_num_args + 2));

    auto const method = gdl::string{"call_deferred"};
    auto const method_name = gdl::variant{gdl::string{"emit_signal"}};

    auto const variant_signal_name = gdl::variant{p_signal_name};

    args[0] = method_name.get();
    args[1] = variant_signal_name.get();

    for (int i = 0; i < p_num_args; i++)
    {
        args[i + 2] = p_args[i];
    }

    godot_variant_call_error error;
    gdl::api->godot_variant_call(&variant, method.get(), args, p_num_args + 2, &error);
    gdl::api->godot_free(args);
    gdl::api->godot_variant_destroy(&variant);

    return error;
}

void get_glyph(
        gd100::terminal const* const term,
        int const row,
        int const column,
        gdl::pool_int_array& line_arr)
{
    auto const glyph = term->screen.get_glyph({column, row});

    auto fg = to_u32(glyph.style.fg);
    auto bg = to_u32(glyph.style.bg);

    if (glyph.style.mode.is_set(gd100::glyph_attr_bit::reversed))
        std::swap(fg, bg);

    line_arr.set(column * 3 + 0, fg);
    line_arr.set(column * 3 + 1, bg);
    line_arr.set(column * 3 + 2, glyph.code);
}

gdl::variant get_line(gd100::terminal const* const term, int const line)
{
    auto const width = term->screen.size().width;

    gdl::pool_int_array line_arr;
    line_arr.resize(width * 3);

    for(int col = 0; col != width; ++col) {
        get_glyph(term, line, col, line_arr);
    }

    return std::move(line_arr);
}

gdl::variant get_lines(gd100::terminal const* const term)
{
    gdl::dictionary line_dict;

    for (int line = 0; line != term->screen.size().height; ++line) {
        auto const& termline = term->screen.lines[line];
        if (!termline.changed)
            continue;

        line_dict.set(std::int64_t{line}, get_line(term, line));
    }

    return gdl::variant{std::move(line_dict)};
}

godot_variant get_cursor(gd100::terminal const* const term)
{
    godot_vector2 cursor_pos;
    gdl::api->godot_vector2_new(&cursor_pos, term->cursor.pos.x, term->cursor.pos.y);

    godot_variant ret;
    gdl::api->godot_variant_new_vector2(&ret, &cursor_pos);

    return ret;
}

std::optional<gdl::variant> lines_key;
std::optional<gdl::variant> cursor_key;
std::optional<gdl::variant> scroll_change_key;

gdl::variant get_terminal_data(const gd100::terminal* const term)
{
    gdl::dictionary term_dict;

    term_dict.set(*lines_key, get_lines(term));
    term_dict.set(*cursor_key, get_cursor(term));
    term_dict.set(*scroll_change_key, std::int64_t{term->screen.changed_scroll()});

    return std::move(term_dict);
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
            terminal.screen.clear_changes();
            const auto* args = data.get();
            object_emit_signal_deferred(
                instance,
                "terminal_updated",
                1,
                &args);
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
        gdl::api->godot_variant_new_nil(&ret);
        return ret;
    }

    auto const code = gdl::api->godot_variant_as_int(args[0]);

    auto term = reinterpret_cast<terminal_program*>(user_data);
    term->send_code(code);

    godot_variant ret;
    gdl::api->godot_variant_new_nil(&ret);
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
        gdl::api->godot_variant_new_nil(&ret);
        return ret;
    }

    auto const mouse_x = gdl::api->godot_variant_as_int(args[0]);
    auto const mouse_y = gdl::api->godot_variant_as_int(args[1]);
    auto const button_nr = gdl::api->godot_variant_as_int(args[2]);
    auto const godot_button = static_cast<godot_mouse_button>(button_nr);
    auto const button = to_terminal_mouse(godot_button);
    auto const pressed = gdl::api->godot_variant_as_bool(args[3]);

    auto term = reinterpret_cast<terminal_program*>(user_data);
    term->process_mouse(mouse_x, mouse_y, button, pressed);

    godot_variant ret;
    gdl::api->godot_variant_new_nil(&ret);
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

        setenv("TERM", "gdterm", 1);

        char command[] = "sh";

        char* const args[]{
            command,
            nullptr,
        };

        execvp(command, args);
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

extern "C" {

void GDTERM_EXPORT godot_gdnative_init(godot_gdnative_init_options* options)
{
    gdl::initialise(options);

    lines_key = gdl::string{"lines"};
    cursor_key = gdl::string{"cursor"};
    scroll_change_key = gdl::string{"scroll_change"};
}

void GDTERM_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options* options)
{
    scroll_change_key.reset();
    cursor_key.reset();
    lines_key.reset();

    gdl::deinitialise();
}

void GDTERM_EXPORT godot_nativescript_init(void* desc)
{
    auto const create = godot_instance_create_func{create_terminal, nullptr, nullptr};
    auto const destroy = godot_instance_destroy_func{destroy_terminal, nullptr, nullptr};

    gdl::nativescript_api->godot_nativescript_register_class(
        desc,
        "TerminalLogic",
        "Reference",
        create, destroy);

    godot_variant nil;
    gdl::api->godot_variant_new_nil(&nil);

    auto signal_arg = godot_signal_argument{
        gdl::api->godot_string_chars_to_utf8("data"),
        GODOT_VARIANT_TYPE_DICTIONARY,
        GODOT_PROPERTY_HINT_NONE,
        gdl::api->godot_string_chars_to_utf8("hint str"),
        GODOT_PROPERTY_USAGE_DEFAULT,
        nil
    };

    auto const signal = godot_signal{
        gdl::api->godot_string_chars_to_utf8("terminal_updated"),
        1, &signal_arg,
        0, nullptr,
    };

    gdl::nativescript_api->godot_nativescript_register_signal(
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

    gdl::nativescript_api->godot_nativescript_register_method(
        desc,
        "TerminalLogic",
        "send_code",
        attr,
        sc_method);

    auto const sm_method = godot_instance_method{
        send_mouse_method,
        nullptr, nullptr,
    };

    gdl::nativescript_api->godot_nativescript_register_method(
        desc,
        "TerminalLogic",
        "send_mouse",
        attr,
        sm_method);
}

}
