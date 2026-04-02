#!/usr/bin/env python3

import tkinter as tk
from tkinter import messagebox, ttk
import configparser
import subprocess
import threading
import os
import sys
import shlex
import ctypes
from enum import Enum
import datetime
import time

from PIL import Image, ImageTk

# TOTP Dependencies
import pyotp
import save_secret

class Lower(Enum):
    IMMEDIATE = 1
    TIMER = 2
    NONE = 3

# location of LANCZOS changed between Pillow 8 and 9
if hasattr(Image, 'Resampling'):
    resample_filter = Image.Resampling.LANCZOS
else:
    resample_filter = Image.LANCZOS

class AppLauncher(tk.Tk):
    """
    A simple graphical application launcher that displays a grid of icons
    based on a configuration file. Now integrated with TOTP functionality.
    """
    def __init__(self, config_path):
        super().__init__()

        self.config_path = config_path
        self.apps_config = []
        self.icon_photo_images = []
        self.time_labels = []
        self.lower_flag = Lower.NONE
        
        # TOTP specific state
        self.tooltip_window = None
        self.clear_timer = None
        self.totp = None

        if not self.load_configuration():
            self.destroy()
            return

        self.title("Application Launcher")
        self.geometry(f"+{self.win_x}+{self.win_y}")

        if self.taskbar_icon:
            if sys.platform == "win32":
                icon_path = os.path.expanduser(os.path.expandvars(self.taskbar_icon))
                if os.path.exists(icon_path):
                    self.iconbitmap(icon_path)
            else:
                icon_path = self.taskbar_icon
                if os.path.exists(icon_path):
                    self.taskbar_photo_image = tk.PhotoImage(file=icon_path)
                    self.iconphoto(True, self.taskbar_photo_image)

        self.configure(bg=self.bgcolor)

        self.create_grid_widgets()

        homedir = os.getenv('USERPROFILE') if sys.platform == "win32" else os.getenv('HOME')
        if homedir:
            os.chdir(homedir)

        self.update_time_labels()

    def load_configuration(self):
        if not os.path.exists(self.config_path):
            messagebox.showinfo(
                "Configuration File Not Found",
                f"'{self.config_path}' was not found.\nSee config-sample.ini"
            )
            return False

        config = configparser.ConfigParser()
        config.read(self.config_path)

        self.rows = config.getint('Global', 'rows', fallback=3)
        self.cols = config.getint('Global', 'columns', fallback=5)
        self.icon_size = config.getint('Global', 'icon_size', fallback=96)
        self.time_font = config.get('Global', 'time_font', fallback="Consolas")
        self.time_font_size = config.getint('Global', 'time_font_size', fallback=12)
        self.time_format = config.get('Global', 'time_format', fallback='%m/%d/%y\n%H:%M:%S\n%A')
        self.time_format = self.time_format.replace('\\n','\n')
        self.padding = config.getint('Global', 'padding', fallback=10)
        self.win_x = config.getint('Global', 'window_x', fallback=200)
        self.win_y = config.getint('Global', 'window_y', fallback=200)
        self.minimize_on_launch = config.getboolean('Global', 'minimize_on_launch', fallback=True)
        self.taskbar_icon = config.get('Global', 'taskbar_icon', fallback=None)

        self.bgcolor = config.get('Global', 'bgcolor', fallback='black')
        self.textcolor = config.get('Global', 'textcolor', fallback='white')
        self.highlight_color = config.get('Global', 'highlight_color', fallback='white')

        # TOTP Settings
        self.totp_secret = config.get('Global', 'totp_secret', fallback=None)
        self.totp_clipboard_time = config.getint('Global', 'totp_clipboard_time', fallback=10)
        self.totp_font_name = config.get('Global', 'totp_font_name', fallback="Consolas")
        self.totp_font_size = config.getint('Global', 'totp_font_size', fallback=24)
        self.totp_bar_width = config.getint('Global', 'totp_bar_width', fallback=64)

        lower = config.get('Global','lower', fallback='notset')
        if lower == 'immediate':
            self.lower_flag = Lower.IMMEDIATE
        elif lower == 'timer':
            self.lower_flag = Lower.TIMER
        elif lower == 'none':
            self.lower_flag = Lower.NONE
        self.lower_time = config.getint('Global', 'lower_time_ms', fallback=1000)

        for r in range(self.rows):
            for c in range(self.cols):
                section = f'Slot_{r}_{c}'
                if config.has_section(section):
                    self.apps_config.append({
                        'title': config.get(section, 'title', fallback=''),
                        'icon': config.get(section, 'icon', fallback=''),
                        'cmd': config.get(section, 'cmd', fallback=''),
                        'startfile': config.get(section, 'startfile', fallback=''),
                    })
                else:
                    self.apps_config.append(None)

        # Override the last two grid slots if TOTP is enabled
        if self.totp_secret and len(self.apps_config) >= 2:
            self.apps_config[-2] = {'cmd': 'TOTP_TIMER'}
            self.apps_config[-1] = {'cmd': 'TOTP_CODE'}

        return True

    def create_grid_widgets(self):
        for i, app_info in enumerate(self.apps_config):
            row = i // self.cols
            col = i % self.cols

            item_frame = tk.Frame(self, bg=self.cget('bg'))
            item_frame.grid(row=row, column=col, padx=self.padding, pady=self.padding, sticky='nsew')

            if app_info and app_info.get('cmd') == 'TIME':
                time_label = tk.Label(item_frame, text="", bg=item_frame.cget('bg'),
                                      fg=self.textcolor, font=(self.time_font, self.time_font_size, "bold"))
                time_label.pack(expand=True, fill='both')
                self.time_labels.append(time_label)

            elif app_info and app_info.get('cmd') == 'TOTP_TIMER':
                self.timer_frame = item_frame

            elif app_info and app_info.get('cmd') == 'TOTP_CODE':
                self.code_frame = item_frame
                self.unlock_btn = tk.Button(item_frame, text="Unlock\nTOTP", 
                                            bg="#333", fg="white", 
                                            font=(self.time_font, self.time_font_size, "bold"),
                                            command=self.unlock_totp)
                self.unlock_btn.pack(expand=True, fill='both', padx=5, pady=5)

            elif app_info and (app_info['cmd'] or app_info['startfile']):
                icon_label = tk.Label(item_frame, bg=item_frame.cget('bg'))
                icon_path = os.path.expanduser(os.path.expandvars(app_info['icon']))
                if os.path.exists(icon_path):
                    img = Image.open(icon_path)
                    img = img.resize((self.icon_size, self.icon_size), resample_filter)
                    photo = ImageTk.PhotoImage(img)
                    icon_label.config(image=photo, height=self.icon_size, width=self.icon_size)
                    self.icon_photo_images.append(photo)
                else:
                    icon_label.config(text="[No Icon]", width=self.icon_size // 6, height=self.icon_size // 20)

                icon_label.pack(pady=(0, 0))

                title_label = tk.Label(item_frame, text=app_info['title'],
                                       bg=item_frame.cget('bg'), fg=self.textcolor)
                title_label.pack()

                widgets_to_bind = [item_frame, icon_label, title_label]
                for widget in widgets_to_bind:
                    widget.bind("<Enter>", lambda evt, f=item_frame: self.on_hover(f))
                    widget.bind("<Leave>", lambda evt, f=item_frame: self.on_leave(f))
                    widget.bind("<Button-1>", lambda evt, cmd=app_info['cmd'], startfile=app_info['startfile']:
                                self.launch_application(cmd, startfile))
            else:
                item_frame.config(width=self.icon_size, height=self.icon_size)
                
            self.bind("<Key-q>", lambda evt: self.destroy())

    def update_time_labels(self):
        current_time = datetime.datetime.now().strftime(self.time_format)
        for label in self.time_labels:
            label.config(text=current_time)
        self.after(1000, self.update_time_labels)

    def on_hover(self, frame):
        frame.config(bg=self.highlight_color)
        for widget in frame.winfo_children():
            widget.config(bg=self.highlight_color)

    def on_leave(self, frame):
        original_color = self.cget('bg')
        frame.config(bg=original_color)
        for widget in frame.winfo_children():
            widget.config(bg=original_color)

    def launch_application(self, command_line, startfile):
        if command_line == "LOWER":
            self.lower()
            return

        if command_line:
            full_command = os.path.expanduser(os.path.expandvars(command_line))
            if full_command.startswith("shell:"):
                SW_SHOWNORMAL = 1
                ctypes.windll.shell32.ShellExecuteW(None, "open", full_command, None, None, SW_SHOWNORMAL)
            else:
                args = shlex.split(full_command)
                subprocess.Popen(args)

        elif startfile:
            def startthread():
                os.startfile(startfile)
            threading.Thread(target=startthread, daemon=True).start()

        if self.minimize_on_launch:
            self.iconify()

        if self.lower_flag == Lower.IMMEDIATE:
            self.lower()
        elif self.lower_flag == Lower.TIMER:
            self.after(self.lower_time, self.lower)

    # --- TOTP Specific Methods ---

    def ask_pin(self):
        pin = None
        dialog = tk.Toplevel(self)
        dialog.title("Unlock")
        dialog.attributes("-topmost", True)
        dialog.resizable(False, False)
        
        tk.Label(dialog, text="Enter TOTP secret pin").pack(padx=20, pady=(10, 5))
        entry = tk.Entry(dialog, show="*", bg="black", fg="white")
        entry.pack(padx=20, pady=(0, 10))
        entry.focus_set()
        
        def on_enter(event=None):
            nonlocal pin
            pin = entry.get()
            dialog.destroy()
            
        def on_esc(event=None):
            dialog.destroy()
            
        entry.bind("<Return>", on_enter)
        dialog.bind("<Escape>", on_esc)
        
        dialog.transient(self)
        dialog.grab_set()
        self.wait_window(dialog)
        
        return pin

    def unlock_totp(self):
        if False:
            # a dummy secret for testing.
            secret = "CLEFDHTP43VLP2UAYYBX532XKLWG6YU4"
        else:
            pin = self.ask_pin()
            if pin is None:
                return
            secret = save_secret.load_secret(pin, self.totp_secret)
            del pin

        self.totp = pyotp.TOTP(secret, interval=60)
        self.unlock_btn.destroy()

        # Initialize the linear progress bar from ttk
        self.progress_style = ttk.Style()
        self.progress_style.configure("black.Horizontal.TProgressbar",
                                      bg="black", fg="green")
        
        self.progress = ttk.Progressbar(
            self.timer_frame,
            orient="horizontal",
            mode="determinate",
            maximum=60,
            style="black.Horizontal.TProgressbar",
            length=self.totp_bar_width,
        )
        self.progress.pack(expand=True,
                           #fill='x',
                           padx=self.padding)

        self.code_var = tk.StringVar()
        self.code_label = tk.Label(self.code_frame, textvariable=self.code_var,
                                   font=(self.totp_font_name, self.totp_font_size, "bold"),
                                   bg=self.cget('bg'), fg=self.textcolor)
        self.code_label.bind("<Button-1>", lambda evt: self.copy_to_clipboard())
        self.code_label.pack(expand=True)

        self.protect_window()
        self.update_totp_ui()

    def update_totp_ui(self):
        current_time = time.time()
        seconds_remaining = 60 - (current_time % 60)
        
        self.progress['value'] = seconds_remaining

        current_code = self.totp.now()
        formatted_code = f"{current_code[:3]}\n{current_code[3:]}"

        if self.code_var.get() != formatted_code:
            self.code_var.set(formatted_code)
            
        self.after(50, self.update_totp_ui)

    def copy_to_clipboard(self):
        v = self.totp.now()

        self.clipboard_clear()
        self.clipboard_append(v)

        if sys.platform.startswith('linux'):
            self.selection_clear(selection='PRIMARY')
            self.selection_own(selection='PRIMARY')
            self.selection_handle(lambda offset, length:
                                  v[int(offset):int(offset)+int(length)],
                                  selection='PRIMARY')

        if self.tooltip_window:
            self.tooltip_window.destroy()
        if self.clear_timer:
            self.after_cancel(self.clear_timer)

        x = self.winfo_pointerx() + 10
        y = self.winfo_pointery() + 10
        
        self.tooltip_window = tk.Toplevel(self)
        self.tooltip_window.wm_overrideredirect(True)
        self.tooltip_window.wm_geometry(f"+{x}+{y}")
        
        label = tk.Label(self.tooltip_window, text="Copied!", bg="#333333", 
                         fg="white", relief="solid", borderwidth=1, padx=4, pady=2)
        label.pack()

        self.clear_timer = self.after(self.totp_clipboard_time * 1000, self.clear_clipboard_and_tooltip)

    def clear_clipboard_and_tooltip(self):
        if self.tooltip_window:
            self.tooltip_window.destroy()
            self.tooltip_window = None
            
        self.clipboard_clear()
        if sys.platform.startswith('linux'):
            self.selection_clear(selection='PRIMARY')

    def protect_window(self):
        if sys.platform.startswith('linux'):
            return
        WDA_MONITOR = 0x00000001
        hwnd = ctypes.windll.user32.GetParent(self.winfo_id())
        ctypes.windll.user32.SetWindowDisplayAffinity(hwnd, WDA_MONITOR)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print('usage:  PfkAppLauncher.py INI_FILE')
        exit(1)
        
    app = AppLauncher(config_path=sys.argv[1])
    if app.winfo_exists():
        app.mainloop()
