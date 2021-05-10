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

        auto const first = static_cast<unsigned char>(consume());

        if (first & 0b1000'0000) {
            return decode_utf8(first);
        }

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

            case '\016': /* SO (LS1 -- Locking shift 1) */
            case '\017': /* SI (LS0 -- Locking shift 0) */
                return {
                    consumed,
                    use_charset_table_instruction{first - '\016'}};

            case esc:
                return decode_escape();
        }

        return {1, write_char_instruction{static_cast<code_point>(first)}};
        // return none_instruction{};
    }

    decode_result decode_utf8(unsigned char const first)
    {
        auto codepoint = std::uint32_t{0};
        auto bytes_left = int{};
        auto bits_received = int{};

        if ((first & 0b1110'0000) == 0b1100'0000) {
            bytes_left = 1;
            bits_received = 5;
        } else if ((first & 0b1111'0000) == 0b1110'0000) {
            bytes_left = 2;
            bits_received = 4;
        } else if ((first & 0b1111'1000) == 0b1111'0000) {
            bytes_left = 3;
            bits_received = 3;
        } else {
            return discard_consumed();
        }

        codepoint = (std::uint32_t{first} & 0xff >> (8 - bits_received)) << bytes_left * 6;

        while(true) {
            if (!characters_left())
                return not_enough_data();

            auto const utf8_part = static_cast<std::uint32_t>(
                                    static_cast<unsigned char>(consume()));

            bytes_left -= 1;
            codepoint |= (utf8_part & 0b0011'1111) << bytes_left * 6;

            if (bytes_left == 0)
                break;
        }

        return {consumed, write_char_instruction{codepoint}};
    }

    decode_result decode_escape()
    {
        if (!characters_left())
            return not_enough_data();

        auto const code = consume();
        switch(code) {
            case 'n': /* LS2 -- Locking shift 2 */
            case 'o': /* LS3 -- Locking shift 3 */
                return {
                    consumed,
                    use_charset_table_instruction{code - 'n' + 2}};

            case '\\': // string terminator
            default:
                return {2, none_instruction()};

            case '(':
            case ')':
            case '*':
            case '+':
                return decode_set_charset_table(code - '(');

            case 'P' : // device control string
            case ']' : // operating system command
            case 'X' : // start of string
            case '^' : // privacy message
            case '_' : // application program command
                return discard_string();

            case 'M':
                return {2, reverse_line_feed_instruction{}};

            case '[':
                return decode_csi();
        }
    }

    decode_result decode_set_charset_table(int table_index)
    {
        if (!characters_left())
            return not_enough_data();

        switch(consume()) {
            case '0':
                return {
                    consumed,
                    set_charset_table_instruction{table_index, charset::graphic0}};

            case 'B':
                return {
                    consumed,
                    set_charset_table_instruction{table_index, charset::usa}};

            default:
                return discard_consumed();
        }
    }

    decode_result discard_string()
    {
        while(true) {
            if (!characters_left())
                return not_enough_data();

            switch (consume()) {
                case '\a':
                    return discard_consumed();

                case esc:
                    if (peek() == '\\') {
                        consume();
                        return discard_consumed();
                    }
            }
        }
    }

    static constexpr int max_csi_params = 10;

    constexpr bool is_csi_argument_character(char const c)
    {
        return c >= 0x30 && c <= 0x3f;
    }

    decode_result decode_csi()
    {
        csi_param params[max_csi_params]{};
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
            return discard_consumed();
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
            if (param_index < param_count &&
                params[param_index].type == csi_param_type::number &&
                params[param_index].type_data.number != 0)
            {
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

            case 'M':
                return {
                    consumed,
                    delete_lines_instruction{get_number(0, 1)}};

            case '@':
                return {
                    consumed,
                    insert_blanks_instruction{get_number(0, 1)}};

            case 'L':
                return {
                    consumed,
                    insert_newline_instruction{get_number(0, 1)}};

            case 'd':
                return {
                    consumed,
                    move_to_row_instruction{get_number(0, 1) - 1}};
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
