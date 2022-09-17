

# best utf8 solution:
# export LC_ALL=en_US.utf8
# NOTE works better if apt/dnf ‘locales’ and ‘locales-all’ are installed

# if that's not available:
    # if you get
    #    "UnicodeEncodeError: 'ascii' codec can't encode character"
    # use:   export PYTHONIOENCODING=utf-8
    # or:
    version = sys.version_info.major + (sys.version_info.minor / 10.0)
    if version >= 3.7:
        sys.stdout.reconfigure(encoding='utf-8')
    else:
        # reconfigure method not supported, so instead:
        sys.stdout = open(sys.stdout.fileno(), mode='w',
                          encoding='utf8', buffering=1)
    # but note this does not solve the problem of os.scandir
    # trying to read a filename that has utf8 encodings in it.
    # only the LC_ALL stuff above fixes that.

# a list of bytes using list comprehension:
out = ['{:02x}'.format(ord(c)) for c in some_string]

# a string of hex digits, i.e. if unicode is involved
out = ''
for c in some_string:
    out = out + '{:04x}'.format(ord(c))
print(out)


# formatting a decimal number with commas !   use {:,}

