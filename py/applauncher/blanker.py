#!/usr/bin/env python3

import tkinter as tk

BACKGROUND_COLOR = "#102030"

root = tk.Tk()
root.title("Blank")
root.geometry("600x400")  # initial size
root.configure(bg=BACKGROUND_COLOR)
# not needed, because default, but just in case.
# root.resizable(True, True)
root.mainloop()
