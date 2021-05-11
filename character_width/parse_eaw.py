import range_list
import parse_util


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
