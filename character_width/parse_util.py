def split_line(line):
    split_result = line.split('#', 1)

    if len(split_result) == 0:
        return None

    data = split_result[0].strip()
    if not data:
        return None

    return data.split(';')


def parse_codepoints(codepoints):
    split = codepoints.split('..')
    if len(split) == 2:
        return int(split[0], base=16), int(split[1], base=16)

    return int(codepoints, base=16), int(codepoints, base=16)
