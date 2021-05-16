#include <utility>
#include <cstring>

#include <catch2/catch.hpp>

#include <gd100/terminal.hpp>
#include <gd100/terminal_decoder.hpp>

struct test_data {
    gd100::terminal t;
    gd100::decoder d;

    void process_bytes(char const* const ptr, std::size_t const count)
    {
        auto instructee = gd100::terminal_instructee{&t};
        d.decode(ptr, count, instructee);
    }
};

// non square terminal by default so that mixups with width/height will be caught.
auto test_term(gd100::extend size={5,4})
{
    return test_data{
            gd100::terminal{size},
            gd100::decoder{}};
}

TEST_CASE("Terminal writing", "[write]") {
    auto tst = test_term();

    SECTION("After initialisation") {
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});
        REQUIRE(tst.t.screen.get_line(3)[4].code == 0);
    }

    SECTION("Writing sets a character and moves the cursor") {
        tst.t.write_char('A');

        REQUIRE(tst.t.screen.get_glyph({0, 0}).code == 'A');
        REQUIRE(tst.t.cursor.pos == gd100::position{1, 0});
    }

    SECTION("Cursor only goes to the next line when placing character there") {
        // Write five times to put the cursor to the end
        tst.t.write_char('A'); tst.t.write_char('B'); tst.t.write_char('C');
        tst.t.write_char('D'); tst.t.write_char('E');

        REQUIRE(tst.t.screen.get_glyph({4, 0}).code == 'E');
        REQUIRE(tst.t.cursor.pos == gd100::position{4, 0});

        tst.t.write_char('F');

        REQUIRE(tst.t.screen.get_glyph({0, 1}).code == 'F');
        REQUIRE(tst.t.cursor.pos == gd100::position{1, 1});
    }
}

TEST_CASE("Terminal scrolling", "[scroll]") {
    auto tst = test_term();

    auto write_full_line = [&] {
        tst.t.write_char('A'); tst.t.write_char('B'); tst.t.write_char('C');
        tst.t.write_char('D'); tst.t.write_char('E');
    };

    SECTION("Should start scrolling when no space left") {
        write_full_line();
        write_full_line();
        write_full_line();
        write_full_line();

        REQUIRE(tst.t.cursor.pos == gd100::position{4, 3});

        tst.t.write_char('a');

        REQUIRE(tst.t.cursor.pos == gd100::position{1, 3});
    }
}

TEST_CASE("Terminal driven via decode", "[terminal-decode]") {
    auto tst = test_term();
    SECTION("Basic write") {
        char hw[] = "Hello world!";
        tst.process_bytes(hw, sizeof(hw) - 1);

        REQUIRE(tst.t.screen.get_glyph({0, 0}).code == 'H');
        REQUIRE(tst.t.screen.get_glyph({4, 0}).code == 'o');
        REQUIRE(tst.t.screen.get_glyph({0, 1}).code == ' ');
        REQUIRE(tst.t.screen.get_glyph({4, 1}).code == 'l');
        REQUIRE(tst.t.screen.get_glyph({0, 2}).code == 'd');
        REQUIRE(tst.t.screen.get_glyph({1, 2}).code == '!');

        REQUIRE(tst.t.cursor.pos == gd100::position{2, 2});
    }
}

TEST_CASE("Backspace", "[backspace]") {
    auto tst = test_term();

    SECTION("Backspace at the start of a line") {
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});
        tst.process_bytes("\b", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});

        tst.process_bytes("\n\b", 2);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 1});
    }

    SECTION("Backspacing over a character") {
        tst.process_bytes("Hey", 3);
        REQUIRE(tst.t.cursor.pos == gd100::position{3, 0});
        tst.process_bytes("\b", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{2, 0});
        tst.process_bytes("\b\b\b\b\b", 5);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});

        REQUIRE(tst.t.screen.get_glyph({1, 0}).code == 'e');
    }

    SECTION("Backspace onto new line") {
        tst.process_bytes("12345", 5); // no space left in the first line
        REQUIRE(tst.t.cursor.pos == gd100::position{4, 0});

        // some shells use this trick to move the cursor to the next line
        tst.process_bytes(" \b", 2);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 1});
    }
}

TEST_CASE("Newline handling", "[newline]") {
    auto tst = test_term();

    SECTION("Newline basics") {
        tst.process_bytes("\n", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 1});
        tst.process_bytes("\n", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 2});
        tst.process_bytes("\n", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 3});
        tst.process_bytes("\n\n\n", 3); // Should start scrolling and leaving cursor alone
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 3});
    }

    SECTION("Newline in middle") {
        tst.process_bytes("123", 3);
        tst.process_bytes("\n", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{3, 1});
    }

    SECTION("Newline at eol") {
        tst.process_bytes("12345", 5);
        tst.process_bytes("\n", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{4, 1});
    }

    SECTION("Newline and carriage return") {
        tst.process_bytes("123", 3);
        SECTION("CRLF") {
            tst.process_bytes("\r\n", 2);
            REQUIRE(tst.t.cursor.pos == gd100::position{0, 1});
        }
        SECTION("LFCR") {
            tst.process_bytes("\n\r", 2);
            REQUIRE(tst.t.cursor.pos == gd100::position{0, 1});
        }
    }
}

TEST_CASE("Carriage return handling", "[carriage-return]") {
    auto tst = test_term();

    tst.process_bytes("abc\r", 4);
    REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});
    REQUIRE(tst.t.screen.get_glyph({0, 0}).code == 'a');
    REQUIRE(tst.t.screen.get_glyph({1, 0}).code == 'b');

    tst.process_bytes("1", 1);
    REQUIRE(tst.t.cursor.pos == gd100::position{1, 0});
    REQUIRE(tst.t.screen.get_glyph({0, 0}).code == '1');
    REQUIRE(tst.t.screen.get_glyph({1, 0}).code == 'b');

    tst.process_bytes("\r", 1);
    REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});
}

TEST_CASE("Tabs", "[tabs]") {
    auto tst = test_term({20, 4});

    SECTION("Cursor positioning") {
        REQUIRE(tst.t.cursor.pos == gd100::position{0, 0});
        tst.process_bytes("\t", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{8, 0});
        tst.process_bytes("\t", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{16, 0});
        tst.process_bytes("Hi", 2);
        REQUIRE(tst.t.cursor.pos == gd100::position{18, 0});
        tst.process_bytes("\t", 1);
        REQUIRE(tst.t.cursor.pos == gd100::position{19, 0});
    }

    SECTION("Tab at end of line") {
        tst.process_bytes("\t\t\t\t", 4);

        REQUIRE(tst.t.cursor.pos == gd100::position{19, 0});

        tst.process_bytes("\tX", 2);
        REQUIRE(tst.t.screen.get_glyph({19, 0}).code == U'X');
        tst.process_bytes("\t€", std::strlen("\t€"));
        REQUIRE(tst.t.screen.get_glyph({19, 0}).code == U'€');
        tst.process_bytes("\t$", 2);
        REQUIRE(tst.t.screen.get_glyph({19, 0}).code == U'$');

        REQUIRE(tst.t.cursor.pos == gd100::position{19, 0});
    }

    SECTION("Tab results") {
        tst.process_bytes("\t", 1); // 0
        REQUIRE(tst.t.cursor.pos.x == 8);

        tst.process_bytes("\r*\t", 3); // 1
        REQUIRE(tst.t.cursor.pos.x == 8);

        tst.process_bytes("\r****\t", 6); // 4
        REQUIRE(tst.t.cursor.pos.x == 8);

        tst.process_bytes("\r*******\t", 9); // 7
        REQUIRE(tst.t.cursor.pos.x == 8);

        tst.process_bytes("\r********\t", 10); // 8
        REQUIRE(tst.t.cursor.pos.x == 16);
    }
}

TEST_CASE("Colour", "[colour]") {
    auto tst = test_term({20, 4});
    tst.process_bytes("\x1b[38;5;57m", 10);
}
