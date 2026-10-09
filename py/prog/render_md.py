#!/usr/bin/env python3

import html
import re
import sys
try:
    from markdown_it import MarkdownIt
    from mdit_py_plugins.dollarmath import dollarmath_plugin
    from latex2mathml.converter import convert as tex2mml
except ImportError:
    print('ERROR: please pip install markdown-it-py linkify-it-py mdit-py-plugins latex2mathml')
    exit(1)

# pip install markdown-it-py linkify-it-py mdit-py-plugins latex2mathml

_TEXT_CMD_RE = re.compile(r"(\\(?:text[a-z]*|mbox|hbox|fbox|emph))\{([^{}]*)\}")
_MTEXT_RE = re.compile(r"(<mtext[^>]*>)(.*?)(</mtext>)")

def _clean_text_cmd(match):
    cmd = match.group(1)
    content = match.group(2)
    # LaTeX text escapes like \_ \% \& \$ \# should produce literal characters in text mode
    content = re.sub(r"\\([_%$&*#])", r"\1", content)
    return f"{cmd}{{{content}}}"

def math_renderer(content, options):
    display_mode = options.get("display_mode", False)
    try:
        cleaned = _TEXT_CMD_RE.sub(_clean_text_cmd, content)
        mml = tex2mml(cleaned, display="block" if display_mode else "inline")
        mml = _MTEXT_RE.sub(lambda m: m.group(1) + m.group(2).replace(r"\_", "_") + m.group(3), mml)
        return mml
    except Exception:
        escaped = html.escape(content)
        delimiter = "$$" if display_mode else "$"
        return f"<code>{delimiter}{escaped}{delimiter}</code>"

def convert(input_file, output_file):
    with open(input_file, "r", encoding="utf-8") as f_input:
        temp_md = f_input.read()

    md = MarkdownIt("gfm-like").use(
        dollarmath_plugin,
        double_inline=True,
        renderer=math_renderer
    )
    # md = MarkdownIt().enable("table")

    try:
        temp_html = md.render(temp_md)
    except Exception as e:
        print(f'ERROR: exception during render. missing linkify-it-py?')
        print(f'EXCEPTION: {e}')
        return 1

    # (note this didn't work well...)
    # 'extra' includes fenced_code, tables, and several other common features
    # 'sane_lists' forces standard list behavior (e.g., ignoring strict blank line rules)
    # temp_html = markdown.markdown(temp_md, extensions=['extra', 'sane_lists'])

    with open(output_file, "w", encoding="utf-8") as f_output:
        f_output.write(temp_html)

    return 0

def main(args):
    if len(args) != 3:
        print('usage: render_md.py input.md output.html')
        return 1
    input_file = args[1]
    output_file = args[2]
    return convert(input_file, output_file)

if __name__ == '__main__':
    exit(main(sys.argv))
