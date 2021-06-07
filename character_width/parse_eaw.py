import range_list
import parse_util


# Code for parsing EastAsianWidth.txt

# Format:
# <codepoint>;<width-category>
# <codepoint>..<codepoint>;<width-category>
#
# 0020;Na
# 0000..001F;N

# The codepoints in the file are in ascending order


def parse(file_name):
    ranges = range_list.RangeList()

    with open(file_name) as data_file:
        for line in data_file:
            data = parse_util.split_line(line)
            if data is None:
                continue

            codepoints, property = data
            codepoints = codepoints.strip()
            property = property.strip()

            range_start, range_end = parse_util.parse_codepoints(codepoints)

            parsed_range = range_list.RangeProperty(range_start, range_end, property)
            ranges.add(parsed_range)

    return ranges
