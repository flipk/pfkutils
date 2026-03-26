#!/usr/bin/env python3

import tkinter as tk
from tkinter import ttk, messagebox
import time
import sys
import pyotp
import save_secret

class TOTPApp:
    def __init__(self, root, secret):
        self.root = root
        self.secret = secret
        self.root.title("TOTP Generator")

        self.left = tk.Frame(root)
        self.left.grid(row=0, column=0, padx=0, pady=0)
        self.right = tk.Frame(root)
        self.right.grid(row=0, column=1, padx=0, pady=0)

        self.root.geometry("270x60") 
        self.root.resizable(False, False)
        self.root.bind("<Key-q>", lambda evt: self.root.destroy())
        
        self.totp = pyotp.TOTP(self.secret, interval=60)
        self.code_var = tk.StringVar()
        
        self.code_label = tk.Label(self.left, textvariable=self.code_var,
                                   font=("Consolas", 34, "bold"))
        self.code_label.bind("<Button-1>",
                             lambda evt: self.copy_to_clipboard())
        self.code_label.pack(padx=0, pady=0)
        
        self.progress = ttk.Progressbar(self.right,
                                        orient="horizontal",
                                        length=100,
                                        mode="determinate",
                                        maximum=60)
        self.progress.pack(padx=0, pady=0)
        
        self.update_ui()

    def copy_to_clipboard(self):
        v = self.code_var.get()

        # 1. Update the CLIPBOARD (Ctrl+V / Right-click Paste)
        self.root.clipboard_clear()
        self.root.clipboard_append(v)

        if sys.platform.startswith('linux'):
            # 2. Update the PRIMARY (Middle-click Paste)
            self.root.selection_clear(selection='PRIMARY')
            self.root.selection_own(selection='PRIMARY')
            self.root.selection_handle(lambda offset, length:
                                  v[int(offset):int(offset)+int(length)],
                                  selection='PRIMARY')

    def update_ui(self):
        current_time = time.time()
        seconds_remaining = 60 - (current_time % 60)
        self.progress['value'] = seconds_remaining
        
        current_code = self.totp.now()
        if self.code_var.get() != current_code:
            self.code_var.set(current_code)
            
        self.root.after(50, self.update_ui)


def ask_pin(root):
    """Creates a modal popup asking for the PIN and returns it."""
    pin = None
    
    dialog = tk.Toplevel(root)
    dialog.title("Unlock")
    dialog.attributes("-topmost", True) # Keep dialog above other windows
    dialog.resizable(False, False)
    
    tk.Label(dialog, text="Enter TOTP secret pin").pack(padx=20, pady=(10, 5))
    
    # show="*" masks the PIN characters
    entry = tk.Entry(dialog, show="*")
    entry.pack(padx=20, pady=(0, 10))
    entry.focus_set()
    
    def on_enter(event=None):
        nonlocal pin
        pin = entry.get()
        dialog.destroy()
        
    def on_esc(event=None):
        dialog.destroy()
        
    # Bind the keys to the Entry widget and the Dialog
    entry.bind("<Return>", on_enter)
    dialog.bind("<Escape>", on_esc)
    
    # Make the dialog act as a modal
    dialog.transient(root)
    dialog.grab_set()
    root.wait_window(dialog) # Pause execution here until dialog is destroyed
    
    return pin

def main(secret_filename):
    root = tk.Tk()

    pin = ask_pin(root)
    if pin is None:
        # The user pressed ESC or closed the window
        sys.exit(0)

    try:
        # Attempt to load and decrypt
        secret = save_secret.load_secret(pin, secret_filename)
    except Exception:
        # Catch decryption failures (like InvalidToken)
        messagebox.showerror("Error", "Incorrect PIN or corrupted data.")
        sys.exit(1)

    # If decryption succeeds, show the main app!
    root.deiconify() 
    app = TOTPApp(root, secret)
    root.mainloop()

if __name__ == "__main__":
    main("totp_secret.bin")
