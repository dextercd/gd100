#include <gd100/terminal_decoder.hpp>

namespace gd100 {

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
        auto instruction = decode_instruction();
        return {consumed, instruction};
    }

    terminal_instruction decode_instruction()
    {
        auto const first = consume();

        switch(first) {
            case '\f':
            case '\v':
            case '\n':
                return line_feed_instruction{};

            case '\r':
                return carriage_return_instruction{};

            case '\b':
                return backspace_instruction{};
        }

        return write_char_instruction{static_cast<code_point>(first)};
        // return none_instruction{};
    }

private:
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
};

decode_result decode(char const* const bytes, int const count)
{
    decoder d{bytes, count};
    return d.decode();
}

} // gd100::
