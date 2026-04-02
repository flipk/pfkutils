#!/usr/bin/env python3

import sys
import tkinter as tk
from PIL import Image, ImageTk
from pathlib import Path

script_path = Path(sys.argv[0]).absolute()
script_dir = script_path.parent
LOWER_IMAGE = script_dir / "lower.png"
BACKGROUND_COLOR = "#102030"
icon_size = 256

root = tk.Tk()
root.title("Blank")
root.geometry("600x400")  # initial size
root.configure(bg=BACKGROUND_COLOR)

# location of LANCZOS changed between Pillow 8 and 9
if hasattr(Image, 'Resampling'):
    resample_filter = Image.Resampling.LANCZOS
else:
    resample_filter = Image.LANCZOS

icon_label = tk.Label(root, bg=BACKGROUND_COLOR)

try:
    img = Image.open(LOWER_IMAGE)
    img = img.resize((icon_size, icon_size), resample_filter)
    photo = ImageTk.PhotoImage(img)
    icon_label.config(image=photo, height=icon_size, width=icon_size)
except:
    # no image, so just put text
    icon_label.config(
        text="icon:\n" \
             f"{LOWER_IMAGE}\n" \
             "couldn't be loaded\n" \
             "click here to lower",
        fg='white')

icon_label.pack(pady=(0, 0))
icon_label.bind("<Button-1>", lambda evt: root.lower())
root.bind("<Key-q>", lambda evt: root.destroy())

root.mainloop()
