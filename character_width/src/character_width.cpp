#include <iterator>
#include <algorithm>

namespace cw {

namespace {

struct code_point_range {
    char32_t begin;
    char32_t end;
};

code_point_range const double_width_ranges[] {

#include <double_width_table.inc>

};

} // anonymous namespace

int character_width(char32_t code)
{
    auto const it = std::lower_bound(
        std::begin(double_width_ranges),
        std::end(double_width_ranges),
        code,
        [](auto const range, auto const code) { return range.end < code; });

    if (it == std::end(double_width_ranges))
        return 1;

    if (code >= it->begin)
        return 2;

    return 1;
}

} // cw::
