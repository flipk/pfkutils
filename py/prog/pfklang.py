#!/usr/bin/env python3

import unicodedata
import sys
import os

# prompt to gemini:
# this command tool will be invoked  in the following ways:
# 1- "language.py check_emoji <string>"  passes the <string> argument to
# contains_emoji and returns an exit code of 0 if True and 1 if False.
# 2- "language.py cvt_code <integer>"  takes <integer> and looks up the
# unicodedata.name and prints it.
# 3- "language.py find_char <string>"  passes the <string> argument to
# find_unicode_by_name, and then prints all the results returned, in the same
# way that test_find_unicode_by_name prints them.
# 4- "language.py check_emoji_dir <dir>"  opens the named directory <dir> and
# recursively looks through all the filenames. for each filename, pass the
# filename to contains_emoji; if that returns True, the full path to the
# filename is printed; if False, print nothing.
# generate a function called "main" that does these four things.
# generate a function called "usage" which shows the user these four ways.
#
# later:
# add a fifth command:  "language.py decompose <string>" which takes the
# contents of <string> one char at a time, looks up the unicodedata.name,
# and prints each char in the same format cvt_code


def contains_junk(text: str) -> bool:
    """
    Checks if the input string contains characters that are often
    considered 'junk' in certain contexts, such as Cyrillic, Greek,
    or extended Latin characters used in Eastern European languages.

    Args:
        text: The string to check.

    Returns:
        True if 'junk' characters are found, False otherwise.
    """
    for c in text:
        # category = unicodedata.category(char)
        name = unicodedata.name(c, "")
        if "CYRILLIC" in name or "GREEK" in name:
            # print(name)
            return True
        # Extended Latin letters commonly used in Eastern European languages
        if 0x0100 <= ord(c) <= 0x01FF:  # Latin Extended-A
            # print(name)
            return True
        if 0x0200 <= ord(c) <= 0x024F:  # Latin Extended-B
            # print(name)
            return True
        if 0x01E00 <= ord(c) <= 0x01EFF:  # Latin Extended Additional
            # print(name)
            return True
    return False


def contains_emoji(text: str) -> bool:
    """
    Checks if the input string contains any emoji characters.

    This function iterates through the input string and checks if the
    Unicode code point of each character falls within common emoji ranges.

    Args:
        text: The string to check for emojis.

    Returns:
        True if an emoji character is found, False otherwise.
    """
    for c in text:
        cp = ord(c)
        # Basic Multilingual Plane (BMP) emoji ranges
        if 0x1F600 <= cp <= 0x1F64F:  # Emoticons
            return True
        elif 0x1F300 <= cp <= 0x1F5FF:  # Miscellaneous Symbols and Pictographs
            return True
        elif 0x1F680 <= cp <= 0x1F6FF:  # Transport and Map Symbols
            return True
        elif 0x1F900 <= cp <= 0x1F9FF:  # Supplemental Symbols and Pictographs
            return True
        elif 0x2600 <= cp <= 0x26FF:  # Miscellaneous Symbols
            return True
        elif 0x2700 <= cp <= 0x27BF:  # Dingbats
            return True
        # Add more ranges as needed
    return False


def find_unicode_by_name(pattern):
    """
    Finds Unicode characters whose names contain the given pattern.

    Args:
        pattern (str): The substring to search for in character names (case-insensitive).

    Returns:
        dict: A dictionary where keys are the matching characters
              and values are their official Unicode names.
              Returns an empty dictionary if no matches are found.
    """
    results = {}
    # Unicode code points range from 0 to 0x10FFFF (1,114,111)
    # sys.maxunicode gives the highest code point supported by this Python build
    # Using 0x110000 covers the entire possible range.
    max_code_point = 0x110000  # sys.maxunicode is often smaller, use full range
    search_pattern = pattern.upper()  # For case-insensitive search

    for i in range(max_code_point):
        try:
            c = chr(i)
            # Get the name, providing an empty default
            # avoids ValueError for unnamed code points
            name = unicodedata.name(c, '')
            if search_pattern in name:
                results[c] = name
        except ValueError:
            # chr(i) might be invalid for some surrogates on narrow builds,
            # though iterating up to 0x10FFFF should generally be safe
            # on modern Python 3 builds.
            # Also handles any unexpected issues retrieving the name.
            pass
        except Exception as e:
            # Catch potential other unexpected errors
            print(f"Unexpected error at codepoint {i}: {e}", file=sys.stderr)

    return results


def test_find_unicode_by_name(pattern_to_find: str):
    heart_chars = find_unicode_by_name(pattern_to_find)

    if heart_chars:
        print(f"Found {len(heart_chars)} characters with '{pattern_to_find}' in their name:")
        # Sort by codepoint for consistent output
        for char in sorted(heart_chars.keys(), key=ord):
            codepoint = ord(char)
            print(f"  U+{codepoint:04X} : {char} : {heart_chars[char]}")
    else:
        print(f"No characters found with '{pattern_to_find}' in their name.")


def test_get_charname(c: int = 129412) -> str:  # example
    name = unicodedata.name(chr(c), '')
    return name


def usage():
    """Prints the usage instructions for the command-line tool."""
    script_name = os.path.basename(sys.argv[0])
    print(f"Usage: {script_name} <command> [arguments]")
    print("\nCommands:")
    print(f"  check_emoji <string>      Checks if <string> contains emoji.\n"
          f"                            Exits 0 if True, 1 if False.")
    print(f"  cvt_code <integer>        Looks up and prints the Unicode name\n"
          f"                            for <integer> codepoint.")
    print(f"  find_char <string>        Finds Unicode characters whose names\n"
          f"                            contain <string> and prints them.")
    print(f"  check_emoji_dir <dir>     Recursively checks filenames in <dir>\n"
          f"                            for emojis and prints matching paths.")
    print(f"  decompose <string>        Decomposes <string> into chars and\n"
          f"                            prints their Unicode names.")
    print("\nExamples:")
    print(f"  {script_name} check_emoji \"Hello 😊\"")
    print(f"  {script_name} cvt_code 128516")
    print(f"  {script_name} find_char \"HEART\"")
    print(f"  {script_name} check_emoji_dir \"./my_documents 😊\"")
    print(f"  {script_name} decompose \"A😊B\"")


def main():
    # Using argparse more conventionally for subcommands would be more robust,
    # but sticking to the current manual parsing style as per the existing code.
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    command = sys.argv[1]
    args_list = sys.argv[2:]

    if command == "check_emoji":
        if not args_list:
            print("Error: <string> argument is required for check_emoji.", file=sys.stderr)
            usage()
            sys.exit(1)
        text_to_check = args_list[0]
        if contains_emoji(text_to_check):
            sys.exit(0)
        else:
            sys.exit(1)
    elif command == "cvt_code":
        if not args_list:
            print("Error: <integer> argument is required for cvt_code.", file=sys.stderr)
            usage()
            sys.exit(1)
        try:
            code_point = int(args_list[0])
            char_val = chr(code_point)
            name = unicodedata.name(char_val, f"No name found for U+{code_point:04X}")
            print(f"U+{code_point:04X} : {char_val} : {name}")
        except ValueError:
            print(f"Error: Invalid integer or codepoint '{args_list[0]}'.", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Error looking up codepoint {args_list[0]}: {e}", file=sys.stderr)
            sys.exit(1)

    elif command == "find_char":
        if not args_list:
            print("Error: <string> argument is required for find_char.", file=sys.stderr)
            usage()
            sys.exit(1)
        pattern = args_list[0]
        results = find_unicode_by_name(pattern)
        if results:
            print(f"Found {len(results)} characters with '{pattern}' in their name:")
            for char_val in sorted(results.keys(), key=ord):
                codepoint = ord(char_val)
                print(f"  U+{codepoint:04X} : {char_val} : {results[char_val]}")
        else:
            print(f"No characters found with '{pattern}' in their name.")
    elif command == "check_emoji_dir":
        if not args_list:
            print("Error: <dir> argument is required for check_emoji_dir.", file=sys.stderr)
            usage()
            sys.exit(1)
        target_dir = args_list[0]
        if not os.path.isdir(target_dir):
            print(f"Error: Directory '{target_dir}' not found.", file=sys.stderr)
            sys.exit(1)
        for root, _, files in os.walk(target_dir):
            for filename in files:
                if contains_emoji(filename):
                    print(os.path.join(root, filename))
    elif command == "decompose":
        if not args_list:
            print("Error: <string> argument is required for decompose.", file=sys.stderr)
            usage()
            sys.exit(1)
        input_string = args_list[0]
        print(f"Decomposing string: \"{input_string}\"")
        for char_val in input_string:
            codepoint = ord(char_val)
            try:
                name = unicodedata.name(char_val)
            except ValueError:
                name = f"No name found for character"
            print(f"  U+{codepoint:04X} : '{char_val}' : {name}")
    else:
        print(f"Error: Unknown command '{command}'.", file=sys.stderr)
        usage()
        sys.exit(1)


if __name__ == "__main__":
    main()
