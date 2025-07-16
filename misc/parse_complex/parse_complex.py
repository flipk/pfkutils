#!/usr/bin/env python3

import re
parser = re.compile(r'^(?:(?:(?:(?:((?:[+-]?)(?:(?:\d+(?:\.\d*)?|\d*\.\d+))))(?:[eE]([+-]?\d+))?)?(?:((?:[+-])(?:(?:\d+(?:\.\d*)?|\d*\.\d+)))(?:[eE]([+-]?\d+))?[ij])?)|(?:(?:((?:[+-]?)(?:(?:\d+(?:\.\d*)?|\d*\.\d+)))(?:(?:[eE]([+-]?\d+)))?[ij])))$')
parser_debug = False

def parse_num(s: str) -> complex:
    r = complex(0)
    m = parser.search(s)
    if not m:
        raise ValueError(f"can't parse complex number: {s}")
    if parser_debug:
        group = 1
        print(f'parsing: {s}')
        for g in m.groups():
            if g:
                print(f'   group {group}: {g}')
            group += 1
    if m.group(1):
        v = float(m.group(1))
        if m.group(2):
            v = v * 10 ** float(m.group(2))
        r += v
    if m.group(3):
        v = float(m.group(3))
        if m.group(4):
            v = v * 10 ** float(m.group(4))
        r += v * 1j
    if m.group(5):
        v = float(m.group(5))
        if m.group(6):
            v = v * 10 ** float(m.group(6))
        r += v * 1j
    return r


def test(s: str):
    res = parse_num(s)
    print(f'parsing: {s:25}   -->  {res}')

test('-4.9e-6+3.1e2j')
test('1+2e6j')
test('-4')
test('-4e3')
test('-4j')
test('-4e3j')
test('4')
test('4e3')
test('4j')
test('4e3j')
