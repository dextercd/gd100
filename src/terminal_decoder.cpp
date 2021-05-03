#include <iostream>
#include <algorithm>
#include <charconv>

#include <gd100/terminal_decoder.hpp>

namespace gd100 {

constexpr char esc = '\x1b';

enum class csi_param_type {
    number,
    character
};

struct csi_param {
    csi_param_type type;
    union {
        int number;
        char character;
    } type_data;

    csi_param()
        : csi_param(0)
    {
    }

    csi_param(int value)
        : type{csi_param_type::number}
    {
        type_data.number = value;
    }

    csi_param(char value)
        : type{csi_param_type::character}
    {
        type_data.character = value;
    }
};

struct decoder {
    const char* bytes;
    int count;
    int consumed = 0;

    decoder(char const* b, int c)
        : bytes{b}
        , count{c}
    {
    }

    decode_result decode()
    {
        if (!characters_left())
            return not_enough_data();

        auto const first = consume();

        switch(first) {
            case '\f':
            case '\v':
            case '\n':
                return {1, line_feed_instruction{}};

            case '\r':
                return {1, carriage_return_instruction{}};

            case '\b':
                return {1, backspace_instruction{}};

            case '\a':
                return {1, none_instruction{}};

            case esc:
                return decode_escape();
        }

        return {1, write_char_instruction{static_cast<code_point>(first)}};
        // return none_instruction{};
    }

    decode_result decode_escape()
    {
        if (!characters_left())
            return not_enough_data();

        switch(consume()) {
            case 'N' : // single shift two
            case 'O' : // single shift three
            case 'P' : // device control string
            case '\\': // string terminator
            case ']' : // operating system command
            case 'X' : // start of string
            case '^' : // privacy message
            case '_' : // application program command
            default:
                return {2, none_instruction()};

            case 'M':
                return {2, reverse_line_feed_instruction{}};

            case '[':
                return decode_csi();
        }
    }

    static constexpr int max_csi_params = 10;

    constexpr bool is_csi_argument_character(char const c)
    {
        return c >= 0x30 && c <= 0x3f;
    }

    decode_result decode_csi()
    {
        csi_param params[10];
        int param_count = 0;

        while(true) {
            if (!characters_left())
                return not_enough_data();

            auto const peeked = peek();

            // Not a parameter, so hand off to next stage
            if (!is_csi_argument_character(peeked))
                return decode_csi_intermediate(params, param_count);

            // Can't store this parameter, so discard it
            if (param_count >= max_csi_params) {
                consume();
                continue; // Process next data
            }

            if (peeked >= '0' && peeked <= '9') {
                int value;
                auto const [consumed_to, err] = std::from_chars(bytes + consumed, bytes + count, value);
                if (err == std::errc::result_out_of_range) {
                    value = -1;
                }

                params[param_count++] = value;
                consumed = consumed_to - bytes;
            } else {
                params[param_count++] = consume();
            }
        }
    }

    static constexpr int max_csi_intermediate = 10;

    constexpr bool is_csi_intermediate(char const c)
    {
        return c >= 0x20 && c <= 0x2f;
    }

    decode_result decode_csi_intermediate(csi_param const* const params, int const param_count)
    {
        char intermediate_bytes[max_csi_intermediate];
        int intermediate_count = 0;

        while(true) {
            if (!characters_left())
                return not_enough_data();

            auto const peeked = peek();

            // Not an csi intermediate byte, so hand off to next stage
            if (!is_csi_intermediate(peeked))
                return decode_csi_finish(params, param_count, intermediate_bytes, intermediate_count);

            if (intermediate_count >= max_csi_intermediate) {
                consume();
                continue;
            }

            intermediate_bytes[intermediate_count++] = consume();
        }
    }

    constexpr bool is_csi_final(char const c)
    {
        return c >= 0x40 && c <= 0x7e;
    }

    constexpr bool is_csi_final_priv(char const c)
    {
        return c >= 0x70 && c <= 0x7e;
    }

    static bool is_csi_param_priv(csi_param const param)
    {
        return param.type == csi_param_type::character
            && param.type_data.character >= 0x3c
            && param.type_data.character <= 0x3f;
    }

    decode_result decode_csi_finish(
            csi_param const* const params,
            int const param_count,
            char const* const intermediate_bytes,
            int const intermediate_count)
    {
        if (!characters_left())
            return not_enough_data();

        auto const final = peek();
        if (!is_csi_final(final)) {
            // Finaly byte in csi sequence is invalid.
            // We discard everything we've consumed so far.
            return {consumed, none_instruction{}};
        }
        consume();

        bool is_priv = is_csi_final_priv(final)
                    || std::any_of(params, params + param_count, is_csi_param_priv);

#if 1
        std::cout << "ESC [";
        for (int i = 0; i < param_count; ++i) {
            std::cout << " ";
            if (params[i].type == csi_param_type::number) {
                std::cout << params[i].type_data.number;
            } else if (params[i].type == csi_param_type::character) {
                std::cout << params[i].type_data.character;
            }
        }

        for (int i = 0; i < intermediate_count; ++i) {
            std::cout << ' ' << intermediate_bytes[i];
        }

        std::cout << ' ' << final << '\n';
#endif

        if (is_priv)
            return decode_csi_priv(params, param_count, intermediate_bytes, intermediate_count, final);

        return decode_csi_pub(params, param_count, intermediate_bytes, intermediate_count, final);
    }

    decode_result decode_csi_priv(
            csi_param const* const params,
            int const param_count,
            char const* const intermediate_bytes,
            int const intermediate_count,
            char const final)
    {
        return discard_consumed();
    }

    decode_result decode_csi_pub(
            csi_param const* const params,
            int const param_count,
            char const* const intermediate_bytes,
            int const intermediate_count,
            char const final)
    {
        auto get_number = [&](int index=0, int default_=0, int offset=0) -> int {
            auto param_index = index * 2 + offset;
            if (param_index < param_count && params[param_index].type == csi_param_type::number) {
                return params[param_index].type_data.number;
            }

            return default_;
        };

        switch(final) {
            default:
                return discard_consumed();

            case 'J': {
                switch(get_number(0)) {
                    case 0:
                        return {consumed, clear_to_bottom_instruction{}};
                    case 1:
                        return {consumed, clear_from_top_instruction{}};

                    case 2:
                    default:
                        return {consumed, clear_screen_instruction{}};
                }
            } break;

            case 'H':
                return {
                    consumed,
                    position_cursor_instruction{{
                        std::max(0, get_number(1) - 1),
                        std::max(0, get_number(0) - 1)
                    }}
                };

            case 'K': {
                switch(get_number(0)) {
                    case 0:
                        return {consumed, clear_to_end_instruction{}};

                    case 1:
                        return {consumed, clear_from_begin_instruction{}};

                    case 2:
                    default:
                        return {consumed, clear_line_instruction{}};
                }
            } break;

            case 'l':
            case 'h': {
                bool set = final == 'h';
                terminal_mode mode;
                for (int i = 0; true; ++i) {
                    auto number = get_number(i, -1);
                    if (number == -1)
                        break;

                    switch(number) {
                        case 4:
                            mode.set(terminal_mode_bit::insert);
                    }
                }

                return {consumed, change_mode_bits_instruction{set, mode}};
            }

            case 'A':
                return {
                    consumed,
                    move_cursor_instruction{get_number(0, 1), direction::up}};

            case 'B':
                return {
                    consumed,
                    move_cursor_instruction{get_number(0, 1), direction::down}};

            case 'C':
                return {
                    consumed,
                    move_cursor_instruction{get_number(0, 1), direction::forward}};

            case 'D':
                return {
                    consumed,
                    move_cursor_instruction{get_number(0, 1), direction::back}};

            case 'P':
                return {
                    consumed,
                    delete_chars_instruction{get_number(0, 1)}};
        }
    }

private:
    decode_result not_enough_data() const
    {
        return {0, none_instruction()};
    }

    decode_result discard_consumed() const
    {
        return {consumed, none_instruction()};
    }

    char consume()
    {
        if (consumed < count)
            return bytes[consumed++];

        return '\0';
    }

    char peek() const
    {
        if (consumed < count)
            return bytes[consumed];

        return '\0';
    }

    int characters_left()
    {
        return count - consumed;
    }
};

decode_result decode(char const* const bytes, int const count)
{
    decoder d{bytes, count};
    return d.decode();
}

} // gd100::
