#!/usr/bin/env python3

import re
from abc import ABC, abstractmethod


class EditDistanceBase(ABC):
    @abstractmethod
    def get_edit_distance_name(self) -> str:
        pass

    @abstractmethod
    def set_edit_distance_value(self, v: int):
        pass


def edit_distance_list(lst: list[EditDistanceBase], str2: str):
    for item in lst:
        str1 = item.get_edit_distance_name()
        v = edit_distance(str1, str2)
        item.set_edit_distance_value(v)


# this is basically the Levenshtein Distance.
# to understand this algorithm, see
# https://en.wikipedia.org/wiki/Levenshtein_distance

def edit_distance(str1: str, str2: str, expected: int = -1) -> int:
    """
    Compares two strings and returns the edit distance between them.
    Ignores non-alphanumeric characters and case.

    Args:
      str1: The first string to compare.
      str2: The second string to compare.
      expected: for unit tests, the expected result;
        -1 means do not check.

    Returns:
      the edit distance (number of insertions, deletions, or edits
      required to get from str1 to str2).
    """

    # Remove non-alphanumeric characters and convert to lowercase
    str1 = re.sub(r'[^a-zA-Z0-9]', '', str1).lower()
    str2 = re.sub(r'[^a-zA-Z0-9]', '', str2).lower()

    # Calculate the Levenshtein distance
    m = len(str1)
    n = len(str2)
    dp = [[0] * (n + 1) for _ in range(m + 1)]
    i = j = 0  # force i and j to be function-local rather than for-loop-local

    for i in range(m + 1):
        for j in range(n + 1):
            if i == 0:
                dp[i][j] = j
                # dp[i][j] = 0  # ignore diffs at start of str2
            elif j == 0:
                dp[i][j] = i
                # dp[i][j] = 0  # ignore diffs at start of str1
            elif str1[i - 1] == str2[j - 1]:
                dp[i][j] = dp[i - 1][j - 1]
            else:
                dp[i][j] = 1 + min(dp[i - 1][j],
                                   dp[i][j - 1],
                                   dp[i - 1][j - 1])

    # this ignores differences at end of either string.
    if m > n:
        for i in range(m, n-1, -1):
            # print(f'comparing dp[{i}][{n}]({dp[i][n]}) to dp[{i-1}][{n}]({dp[i-1][n]})')
            if dp[i][n] <= dp[i-1][n]:
                break
        ret = dp[i][j]
        # print(f'stopped at i={i} ({ret})')
    else:
        for j in range(n, m-1, -1):
            # print(f'comparing dp[{m}][{j}]({dp[m][j]}) to dp[{m}][{j-1}]({dp[m][j-1]})')
            if dp[m][j] <= dp[m][j-1]:
                break
        ret = dp[m][j]
        # print(f'stopped at j={j} ({ret})')

    if expected != -1 and expected != ret:
        print('     j:   ', end='')
        for j in range(1, n + 1):
            print(f'{str2[j - 1]}  ', end='')
        print('')
        print('         ', end='')
        for j in range(1, n + 1):
            print(f'{j:2d} ', end='')
        print('')
        for i in range(m + 1):
            if i == 0:
                print(' i    ', end='')
            else:
                print(f'{i:2d}:{str1[i-1]}: ', end='')
            for j in range(n + 1):
                print(f'{dp[i][j]:2d} ', end='')
            print('')

    return ret


def main():
    def test(s1: str, s2: str, expected: int = -1):
        dist = edit_distance(s1, s2, expected)
        print(f'"{s1}" -> "{s2}" = {dist} (expected: {expected})')

    test('pattern one', 'PATTERN-ONE', 0)
    test('pattern 0ne', 'PATTERN-ONE', 1)

    # differences-at-beginning not supported for now
    # test('xxxpattern one', 'PATTERN-ONE', 0)
    # test('xxxpattern 0ne', 'PATTERN-ONE', 1)
    # test('pattern one', 'XXXPATTERN-ONE', 0)
    # test('pattern 0ne', 'XXXPATTERN-ONE', 1)

    # differences-at-end are supported!
    test('pattern onexxx', 'PATTERN-ONE', 0)
    test('pattern 0nexxx', 'PATTERN-ONE', 1)
    test('pattern one', 'PATTERN-ONEXXX', 0)
    test('pattern 0ne', 'PATTERN-ONEXXX', 1)


if __name__ == '__main__':
    exit(main())
