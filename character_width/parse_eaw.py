import range_list


def parse_codepoints(codepoints):
    split = codepoints.split('..')
    if len(split) == 2:
        return int(split[0], base=16), int(split[1], base=16)

    return int(codepoints, base=16), int(codepoints, base=16)


def parse(file_name):
    ranges = range_list.RangeList()

    with open(file_name) as data_file:
        for line in data_file:
            split_result = line.split('#')

            if len(split_result) == 0:
                continue # Empty line

            data = split_result[0].strip()
            if not data:
                continue # Only comment on this line

            codepoints, property = data.split(';')
            codepoints = codepoints.strip()
            property = property.strip()

            range_start, range_end = parse_codepoints(codepoints)

            parsed_range = range_list.RangeProperty(range_start, range_end, property)
            ranges.add(parsed_range)

    return ranges
