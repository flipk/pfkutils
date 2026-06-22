#!/usr/bin/env python3

# alternative: use github markdown preview tool
# pip install grip
# grip README.md
# visit http://localhost:6419

import sys
import markdown

def convert(input_file, output_file):

    # Read from input file, convert, and write to output file
    with open(input_file, "r", encoding="utf-8") as f_input:
        temp_md = f_input.read()

    temp_html = markdown.markdown(temp_md, extensions=['fenced_code'])

    with open(output_file, "w", encoding="utf-8") as f_output:
        f_output.write(temp_html)

def main(args):
    if len(args) != 3:
        print('usage: markdown input.md output.html')
        return 1
    input_file = args[1]
    output_file = args[2]
    convert(input_file, output_file)
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
