#!/usr/bin/env python3

# ub: apt install zbar-tools oathtool
# fed: dnf install zbar oathtool
# zbarimg -q --nodbus --raw qr.png
# oathtool --totp -b --time-step-size=60s "$secret"
# Where ‘secret’ is 32 character base32
# someday: system tray icon on windows?

import tkinter as tk
from tkinter import ttk, messagebox
import time
import sys
import save_secret
import ctypes

try:
    import pyotp
except:
    if sys.platform.startswith('linux'):
        print('please install package pyotp')
        exit(1)
    else:
        ctypes.windll.user32.MessageBoxW(
            0, "Please install package 'pyotp'.",
            "Import Error", 0x10)
        exit()

class TOTPApp:
    def __init__(self, root, secret):
        self.root = root
        self.secret = secret
        self.root.title("TOTP Generator")
        self.root.configure(bg="black")

        self.left = tk.Frame(root)
        self.left.grid(row=0, column=0, padx=0, pady=0)
        self.right = tk.Frame(root)
        self.right.grid(row=0, column=1, padx=0, pady=0)

        self.root.geometry("230x40")
        self.root.resizable(False, False)
        self.root.bind("<Key-q>", lambda evt: self.root.destroy())
        
        self.totp = pyotp.TOTP(self.secret, interval=60)
        self.code_var = tk.StringVar()
        
        self.code_label = tk.Label(self.left, textvariable=self.code_var,
                                   font=("Consolas", 24, "bold"),
                                   bg="black", fg="white")
        self.code_label.bind("<Button-1>",
                             lambda evt: self.copy_to_clipboard())
        self.code_label.pack(padx=0, pady=0)

        self.progress_style = ttk.Style()
        self.progress_style.configure("black.Horizontal.TProgressbar",
                                      bg="black", fg="green")
        self.progress = ttk.Progressbar(self.right,
                                        orient="horizontal",
                                        length=100,
                                        mode="determinate",
                                        maximum=60,
                                        style="black.Horizontal.TProgressbar")
        self.progress.pack(padx=0, pady=0)

        # tracking variables for the tooltip and timer
        self.tooltip_window = None
        self.clear_timer = None

        self.update_ui()

    def copy_to_clipboard(self):
        v = self.code_var.get()

        # 1. Update the CLIPBOARD
        self.root.clipboard_clear()
        self.root.clipboard_append(v)

        # 2. Update the PRIMARY selection (Linux)
        if sys.platform.startswith('linux'):
            self.root.selection_clear(selection='PRIMARY')
            self.root.selection_own(selection='PRIMARY')
            self.root.selection_handle(lambda offset, length:
                                  v[int(offset):int(offset)+int(length)],
                                  selection='PRIMARY')

        # 3. Reset existing tooltip and timer if user clicks multiple times
        if self.tooltip_window:
            self.tooltip_window.destroy()
        if self.clear_timer:
            self.root.after_cancel(self.clear_timer)

        # 4. Create the "Copied!" tooltip
        x = self.root.winfo_pointerx() + 10
        y = self.root.winfo_pointery() + 10
        
        self.tooltip_window = tk.Toplevel(self.root)
        self.tooltip_window.wm_overrideredirect(True) # Removes window decorations/title bar
        self.tooltip_window.wm_geometry(f"+{x}+{y}")
        
        label = tk.Label(self.tooltip_window, text="Copied!", bg="#333333", 
                         fg="white", relief="solid", borderwidth=1, padx=4, pady=2)
        label.pack()

        # 5. Set the 10-second (10000ms) timer
        self.clear_timer = self.root.after(10000, self.clear_clipboard_and_tooltip)

    def clear_clipboard_and_tooltip(self):
        # Destroy the tooltip
        if self.tooltip_window:
            self.tooltip_window.destroy()
            self.tooltip_window = None
            
        # Clear clipboards
        self.root.clipboard_clear()
        if sys.platform.startswith('linux'):
            self.root.selection_clear(selection='PRIMARY')

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
    entry = tk.Entry(dialog, show="*", bg="black", fg="white")
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

def protect_window(root):
    if sys.platform.startswith('linux'):
        # no such ctypes API
        return

    # Constants for Windows Display Affinity
    WDA_NONE = 0x00000000
    WDA_MONITOR = 0x00000001  # Use this to hide from screenshots

    # Get the window handle (HWND)
    hwnd = ctypes.windll.user32.GetParent(root.winfo_id())

    # Set the affinity to WDA_MONITOR
    # This prevents the window content from being captured
    ctypes.windll.user32.SetWindowDisplayAffinity(hwnd, WDA_MONITOR)

def main(secret_filename):
    root = tk.Tk()

    if True:
        # a dummy secret for testing.
        secret = "CLEFDHTP43VLP2UAYYBX532XKLWG6YU4"
    else:
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

    app = TOTPApp(root, secret)
    root.after(100, lambda: protect_window(root))
    root.mainloop()

if __name__ == "__main__":
    main("totp_secret.bin")
