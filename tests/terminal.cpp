#include <catch2/catch.hpp>

#include <gd100/terminal.hpp>

terminal test_term()
{
    // non square so that mixups with width/height will be caught.
    return terminal{{5, 4}};
}

TEST_CASE("Terminal writing", "[write]") {
    auto t = test_term();
    REQUIRE(t.cursor.pos == position{0, 0});

    SECTION("Writing sets a character and moves the cursor") {
        t.write_char('A');

        REQUIRE(t.screen.get_glyph({0, 0}).code == 'A');
        REQUIRE(t.cursor.pos == position{1, 0});
    }

    SECTION("Cursor only goes to the next line when placing character there") {
        // Write five times to put the cursor to the end
        t.write_char('A'); t.write_char('B'); t.write_char('C');
        t.write_char('D'); t.write_char('E');

        REQUIRE(t.screen.get_glyph({4, 0}).code == 'E');
        REQUIRE(t.cursor.pos == position{4, 0});

        t.write_char('F');

        REQUIRE(t.screen.get_glyph({0, 1}).code == 'F');
        REQUIRE(t.cursor.pos == position{1, 1});
    }
}

TEST_CASE("Terminal scrolling", "[scroll]") {
    auto t = test_term();

    auto write_full_line = [&] {
        t.write_char('A'); t.write_char('B'); t.write_char('C');
        t.write_char('D'); t.write_char('E');
    };

    SECTION("Should start scrolling when no space left") {
        write_full_line();
        write_full_line();
        write_full_line();
        write_full_line();

        REQUIRE(t.cursor.pos == position{4, 3});

        t.write_char('a');

        REQUIRE(t.cursor.pos == position{1, 3});
    }
}
