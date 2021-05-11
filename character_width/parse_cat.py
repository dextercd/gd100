import range_list
import parse_util


def parse(file_name):
    unordered_ranges = []

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
            unordered_ranges.append(parsed_range)

    ranges = range_list.RangeList()
    for range in sorted(unordered_ranges, key=lambda r: r.range_start):
        ranges.add(range)

    return ranges
