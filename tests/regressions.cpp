#include <cstdint>

#include <catch2/catch.hpp>

#include <gd100/terminal.hpp>
#include <gd100/terminal_decoder.hpp>

template<std::size_t N>
void run(unsigned char (&crash_case)[N])
{
    gd100::decoder decoder;
    gd100::terminal term{{132, 80}};
    gd100::terminal_instructee instructee{&term};
    decoder.decode(
        reinterpret_cast<const char*>(crash_case),
        N, instructee);
}

TEST_CASE("Negative scroll", "[regression][scroll]") {
    // Bug where the parser was decoding negative numbers.
    // Control sequences can not be less than zero.
    unsigned char crash_negative_scroll[] = {
      0x0c, 0x1b, 0x5b, 0x2d, 0x37, 0x4d
    };
    run(crash_negative_scroll);
}

TEST_CASE("Large scroll down", "[regression][scroll]") {
    // With a large scroll down the rotate call was given a point to
    // scroll to outside of the begin(), end()
    unsigned char large_scroll_down[] = {
      0x0b, 0x1b, 0x5b, 0x33, 0x33, 0x37, 0x4c
    };
    run(large_scroll_down);
}
