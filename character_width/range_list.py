from dataclasses import dataclass, replace


@dataclass(frozen=True)
class RangeProperty:
    range_start: int
    range_end: int
    property: any

    def __post_init__(self):
        if self.range_start > self.range_end:
            raise Exception("Invalid range.")

    def can_be_merged(self, other):
        if self.property != other.property:
            return False

        return not(
            self.range_end + 1 < other.range_start or
            other.range_end + 1 < self.range_start)

    def merge(self, other):
        """Creates a range that encompasses both self and other

        Doesn't check whether the ranges can be merged.
        You can check that via can_be_merged.
        """
        return RangeProperty(
                    range_start=min(self.range_start, other.range_start),
                    range_end=max(self.range_end, other.range_end),
                    property=self.property)


class RangeList:
    def __init__(self):
        self._completed_ranges = []
        self._in_progress_range = None

    def add(self, new_range):
        if self._in_progress_range is None:
            self._in_progress_range = new_range
            return

        if new_range.range_start <= self._in_progress_range.range_end:
            raise Exception('Items must be added in order.')

        if self._in_progress_range.can_be_merged(new_range):
            self._in_progress_range = self._in_progress_range.merge(new_range)
            return

        self._completed_ranges.append(self._in_progress_range)
        self._in_progress_range = new_range

    def __iter__(self):
        yield from self._completed_ranges
        if self._in_progress_range is not None:
            yield self._in_progress_range


def transform_list(range_list, property_mapping):
    new_list = RangeList()
    for range in range_list:
        new_range = replace(range, property=property_mapping(range.property))
        new_list.add(new_range)

    return new_list
