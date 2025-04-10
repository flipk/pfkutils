#!/usr/bin/env python3

from generate_tone import generate_tone
from upsample_and_filter import upsample_and_filter

# from generate_cosine_table import generate_cosine_table
# table = generate_cosine_table(14, 8192)
# count=0
# for v in table:
#     print(f'{count}  {v}')
#     count += 1

tone = generate_tone(48000, 440,
                     1024,
                     12, 2048)

# count=0
# for v in tone:
#     print(f'{count}  {v}')
#     count += 1

ustone = upsample_and_filter(tone)

count=0
for v in ustone:
    print(f'{count}  {v}')
    count += 1
