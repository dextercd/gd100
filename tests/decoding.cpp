#include <catch2/catch.hpp>

#include <gd100/terminal_decoder.hpp>

TEST_CASE("utf-8", "[utf-8]") {
    SECTION("Incomplete sequence is not consumed") {
        auto decoded = gd100::decode("\xe2\x94", 2);
        REQUIRE(decoded.bytes_consumed == 0);
        REQUIRE(decoded.instruction.type == gd100::instruction_type::none);
    }

    SECTION("Check decodings") {
        auto decode_utf8 = [](auto const& utf8) {
            auto decoded = gd100::decode(utf8, sizeof(utf8) - 1);
            REQUIRE(decoded.bytes_consumed == sizeof(utf8) - 1);
            REQUIRE(decoded.instruction.type == gd100::instruction_type::write_char);
            return decoded.instruction.write_char.code;
        };

        REQUIRE(decode_utf8("k") == U'k');
        REQUIRE(decode_utf8("ē") == U'ē');
        REQUIRE(decode_utf8("¥") == U'¥');
        REQUIRE(decode_utf8("│") == U'│');
        REQUIRE(decode_utf8("¢") == U'¢');
        REQUIRE(decode_utf8("ह") == U'ह');
        REQUIRE(decode_utf8("€") == U'€');
        REQUIRE(decode_utf8("한") == U'한');
        REQUIRE(decode_utf8("𐍈") == U'𐍈');
        REQUIRE(decode_utf8("⚡") == U'⚡');
    }
}
