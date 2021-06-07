def split_line(line):
    """Splits a line in Unicode's .txt files into its components.

    Splits the input by semicolon.

    :returns:
        None if the line contains no components,
        otherwise returns a list of strings.
    """
    split_result = line.split('#', 1)

    if len(split_result) == 0:
        return None

    data = split_result[0].strip()
    if not data:
        return None

    return data.split(';')


def parse_codepoints(codepoints):
    """Parses codepoint ranges in the format of Unicode's .txt files.

    :param codepoints: String specifying codepoints

    Parses these two styles:

    - 038B
    - 0380..0383

    :returns: Two int values forming an inclusive range of code points.
    """
    split = codepoints.split('..')
    if len(split) == 2:
        return int(split[0], base=16), int(split[1], base=16)

    return int(codepoints, base=16), int(codepoints, base=16)
