#!/usr/bin/env python3

import re


# this is basically the Levenshtein Distance.
# to understand this algorithm, see
# https://en.wikipedia.org/wiki/Levenshtein_distance

def edit_distance(str1: str, str2: str, expected: int = -1) -> int:
    """
    Compares two strings and returns the edit distance between them.
    Ignores non-alphanumeric characters and case, as well as suffixes
    on either of the strings.

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

    # Calculate the edit distance using dynamic programming
    m = len(str1)
    n = len(str2)
    dp = [[0] * (n + 1) for _ in range(m + 1)]

    for i in range(m + 1):
        for j in range(n + 1):
            if i == 0:
                # dp[i][j] = j
                dp[i][j] = 0  # ignore diffs at start of str2
            elif j == 0:
                # dp[i][j] = i
                dp[i][j] = 0  # ignore diffs at start of str1
            elif str1[i - 1] == str2[j - 1]:
                dp[i][j] = dp[i - 1][j - 1]
            else:
                dp[i][j] = 1 + min(dp[i - 1][j],
                                   dp[i][j - 1],
                                   dp[i - 1][j - 1])

    # the above code ignores diffs at the start, but now we have to
    # ignore diffs at the ends. start at dp[m][n] and decide first
    # do we go up [m-1][n], or left [m][n-1]. when we decide, then
    # start moving in that direction looking for a minimum. if the
    # values start going back up again, stop and return the min.

    i = m
    j = n
    if dp[i-1][j] < dp[i][j]:
        while dp[i-1][j] < dp[i][j]:
            i -= 1
    else:
        while dp[i][j-1] < dp[i][j]:
            j -= 1
    ret = dp[i][j]
    if expected != -1 and expected != ret:
        print('   j:', end='')
        for j in range(1, n + 1):
            print(f'{str2[j - 1]} ', end='')
        print('')
        for i in range(m + 1):
            if i == 0:
                print('i: ', end='')
            else:
                print(f'{str1[i-1]}: ', end='')
            for j in range(n + 1):
                print(f'{dp[i][j]} ', end='')
            print('')
        print(f'settled on i={i}, j={j}, ret={ret}')

    return ret


def main():
    def test(s1: str, s2: str, expected: int = -1):
        dist = edit_distance(s1, s2, expected)
        print(f'"{s1}" -> "{s2}" = {dist} (expected: {expected})')

    test('pattern one', 'PATTERN-ONE', 0)
    test('pattern 0ne', 'PATTERN-ONE', 1)
    test('xxxpattern one', 'PATTERN-ONE', 0)
    test('xxxpattern 0ne', 'PATTERN-ONE', 1)
    test('pattern one', 'XXXPATTERN-ONE', 0)
    test('pattern 0ne', 'XXXPATTERN-ONE', 1)
    test('pattern onexxx', 'PATTERN-ONE', 0)
    test('pattern 0nexxx', 'PATTERN-ONE', 1)
    test('pattern one', 'PATTERN-ONEXXX', 0)
    test('pattern 0ne', 'PATTERN-ONEXXX', 1)


if __name__ == '__main__':
    exit(main())
