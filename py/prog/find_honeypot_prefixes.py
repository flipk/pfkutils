#!/usr/bin/env python3

import os
import re
import sys
from collections import defaultdict

# add the following to bashrc:
#
# ignore_patterns="$(${HOME}/pfk/bin/find_honeypot_prefixes.py)"
# alias ls="ls $ignore_patterns"
# dir () 
# { 
#     ls -F -n "$@" | $__sed 's, ->.*, @,'
# }

def get_honeypot_patterns(directory="~/Desktop"):
    # Expand ~ to the user's home directory
    dir_path = os.path.expanduser(directory)
    
    try:
        # os.listdir uses readdir and does NOT stat() the files, 
        # safely bypassing the ESEARCH errors from the honeypots.
        filenames = os.listdir(dir_path)
    except OSError as e:
        print(f"Error reading directory {dir_path}: {e}", file=sys.stderr)
        return []

    # print(filenames)
    prefixes = defaultdict(int)

    for filename in filenames:
        if len(filename) > 6:
            prefix = filename[:5]
            prefixes[prefix] += 1

    #  print(prefixes)

    # Filter for patterns that have 8 or more files associated with them
    honeypot_prefixes = [prefix for prefix, count in prefixes.items() if count >= 8]
    return honeypot_prefixes

def main():
    prefixes = get_honeypot_patterns("~/Desktop")
    
    if not prefixes:
        # Output nothing if no patterns are found, keeping the alias clean
        return

    args = []
    for prefix in prefixes:
        # Escape backslashes, exclamation marks, and handle single quotes 
        # to ensure it behaves safely when evaluated by bash.
        escaped_prefix = prefix.replace('\\', '\\\\').replace('!', '\\!').replace("'", "'\\''")
        args.append(f"-I '{escaped_prefix}*'")
        
    # Print the final space-separated string to stdout
    print(" ".join(args))

if __name__ == "__main__":
    main()

