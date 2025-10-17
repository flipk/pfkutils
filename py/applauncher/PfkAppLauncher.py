#!/usr/bin/env python3

import tkinter as tk
from tkinter import messagebox
import configparser
import subprocess
import threading
import os
import sys
import shlex
import ctypes
from enum import Enum
import datetime

# Attempt to import Pillow (PIL). Provide guidance if it's not installed.
try:
    # noinspection PyPackageRequirements,PyUnresolvedReferences
    from PIL import Image, ImageTk
except ImportError:
    # Use a simple print statement for the error, as tkinter may not be
    # available in a context where this script is first run.
    print("ERROR: The 'Pillow' library is required to run this application.")
    print("Please install it by running: pip install Pillow")
    sys.exit(1)


class Lower(Enum):
    IMMEDIATE = 1
    TIMER = 2
    NONE = 3

class AppLauncher(tk.Tk):
    """
    A simple graphical application launcher that displays a grid of icons
    based on a configuration file.
    """
    def __init__(self, config_path):
        super().__init__()

        self.config_path = config_path
        self.apps_config = []
        self.icon_photo_images = []  # Must keep a reference to images
        self.time_labels = []  # To hold labels that display the time
        self.lower_flag = Lower.NONE

        if not self.load_configuration():
            # If config loading fails (e.g., file not found and couldn't be created),
            # destroy the window and exit.
            self.destroy()
            return

        # Configure the main window
        self.title("Application Launcher")
        self.geometry(f"+{self.win_x}+{self.win_y}")
        # self.resizable(False, False)

        # --- Set Taskbar Icon (Platform Specific) ---
        # This must be done after the window is initialized but before the main loop.
        if self.taskbar_icon:
            if sys.platform == "win32":
                try:
                    icon_path = os.path.expanduser(os.path.expandvars(self.taskbar_icon))
                    if os.path.exists(icon_path):
                        self.iconbitmap(icon_path)
                    else:
                        print(f"Warning: Taskbar icon not found at '{icon_path}'")
                except Exception as e:
                    print(f"Warning: Failed to set taskbar icon. {e}")
            else:
                try:
                    icon_path = self.taskbar_icon
                    if not os.path.exists(icon_path):
                        print(f"Warning: Taskbar icon not found at '{icon_path}'")
                    else:
                        # use iconphoto with a PNG/GIF.
                        # We must keep a reference to this image to prevent it from being
                        # garbage collected.
                        self.taskbar_photo_image = tk.PhotoImage(file=icon_path)
                        self.iconphoto(True, self.taskbar_photo_image)
                except Exception as e:
                    print(f"Warning: Failed to set taskbar icon. {e}")

        # Set the background color. 'SystemButtonFace' is a default system color.
        self.configure(bg=self.bgcolor)

        # Create the grid of application icons
        self.create_grid_widgets()

        homedir = None
        if sys.platform == "win32":
            # launched apps should start in USERPROFILE directory
            homedir = os.getenv('USERPROFILE')
        else:
            # launched apps should start in $HOME
            homedir = os.getenv('HOME')
        if homedir:
            os.chdir(homedir)

        # Start the clock update loop
        self.update_time_labels()

    # noinspection PyAttributeOutsideInit
    def load_configuration(self):
        """
        Loads settings from the INI file. If the file doesn't exist,
        it creates a default one.
        """
        if not os.path.exists(self.config_path):
            messagebox.showinfo(
                "Configuration File Not Found",
                f"'{self.config_path}' was not found.\n"
                "See config-sample.ini"
            )
            return False

        config = configparser.ConfigParser()
        config.read(self.config_path)

        # Load settings from the [Global] section with sane defaults
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

        lower = config.get('Global','lower', fallback='notset')
        if lower == 'immediate':
            self.lower_flag = Lower.IMMEDIATE
        elif lower == 'timer':
            self.lower_flag = Lower.TIMER
        elif lower == 'none':
            self.lower_flag = Lower.NONE
        else:
            messagebox.showinfo(
                "Config file error",
                "Global.lower must be one of immediate, timer, or none"
            )
            return False

        # Load settings for each application slot
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
                    # Use None as a placeholder for an empty grid slot
                    self.apps_config.append(None)
        return True

    def create_grid_widgets(self):
        """
        Populates the main window with frames, icons, and labels for each app.
        """
        for i, app_info in enumerate(self.apps_config):
            row = i // self.cols
            col = i % self.cols

            # Each item is a Frame containing an icon Label and a text Label
            item_frame = tk.Frame(self, bg=self.cget('bg'))
            item_frame.grid(row=row, column=col,
                            padx=self.padding, pady=self.padding,
                            sticky='nsew')

            # Special case for displaying the time
            if app_info and app_info.get('cmd') == 'TIME':
                time_label = tk.Label(item_frame,
                                      text="",  # Initial text, will be updated
                                      bg=item_frame.cget('bg'),
                                      fg=self.textcolor,
                                      font=(self.time_font,
                                            self.time_font_size,
                                            "bold"))
                time_label.pack(expand=True, fill='both')
                self.time_labels.append(time_label)

            elif app_info and (app_info['cmd'] or app_info['startfile']):
                # --- Icon Label ---
                icon_label = tk.Label(item_frame, bg=item_frame.cget('bg'))
                try:
                    # Expand environment variables and user home directory shortcuts
                    icon_path = os.path.expanduser(os.path.expandvars(app_info['icon']))
                    if os.path.exists(icon_path):
                        # Open image with Pillow
                        img = Image.open(icon_path)
                        # Resize the image smoothly to the configured size
                        img = img.resize((self.icon_size, self.icon_size), Image.Resampling.LANCZOS)
                        # Convert to a PhotoImage that tkinter can use
                        photo = ImageTk.PhotoImage(img)
                        # noinspection PyTypeChecker
                        icon_label.config(image=photo, height=self.icon_size, width=self.icon_size)
                        # IMPORTANT: Keep a reference to the PhotoImage to prevent garbage collection
                        self.icon_photo_images.append(photo)
                    else:
                        # Show text if icon is not found
                        icon_label.config(text="[No Icon]", width=self.icon_size // 6, height=self.icon_size // 20)
                except Exception as e:
                    print(f"Error loading icon for {app_info['title']}: {e}")
                    icon_label.config(text="[Bad Icon]", width=self.icon_size // 6, height=self.icon_size // 20)

                icon_label.pack(pady=(0, 0))

                # --- Title Label ---
                title_label = tk.Label(item_frame,
                                       text=app_info['title'],
                                       bg=item_frame.cget('bg'),
                                       fg=self.textcolor)
                title_label.pack()

                # --- Bind Events ---
                # We use a lambda to pass the specific frame and command to the handlers.
                # All widgets inside the frame should trigger the launch and hover effects.
                widgets_to_bind = [item_frame, icon_label, title_label]
                for widget in widgets_to_bind:
                    widget.bind("<Enter>", lambda evt, f=item_frame: self.on_hover(f))
                    widget.bind("<Leave>", lambda evt, f=item_frame: self.on_leave(f))
                    widget.bind("<Button-1>", lambda
                                evt,
                                cmd=app_info['cmd'],
                                startfile=app_info['startfile']:
                                self.launch_application(cmd, startfile))
            else:
                # This is an empty slot, create a blank frame to maintain grid structure
                item_frame.config(width=self.icon_size, height=self.icon_size)
            self.bind("<Key-q>", lambda evt: self.destroy())

    def update_time_labels(self):
        """Updates all time labels with the current time and reschedules itself."""
        current_time = datetime.datetime.now().strftime(self.time_format)
        for label in self.time_labels:
            label.config(text=current_time)
        # Schedule the next update in 1000ms (1 second)
        self.after(1000, self.update_time_labels)

    def on_hover(self, frame):
        """Changes background color of a grid item on mouse-over."""
        frame.config(bg=self.highlight_color)
        for widget in frame.winfo_children():
            widget.config(bg=self.highlight_color)

    def on_leave(self, frame):
        """Resets background color when the mouse leaves a grid item."""
        original_color = self.cget('bg')
        frame.config(bg=original_color)
        for widget in frame.winfo_children():
            widget.config(bg=original_color)

    def launch_application(self, command_line, startfile):
        """
        Executes the program specified in the config and optionally minimizes the launcher.
        """
        if command_line == "LOWER":
            self.lower()
            return
        path = "[None]"
        try:
            if command_line:
                # Expand environment variables and user home directory shortcuts in the command
                full_command = os.path.expanduser(os.path.expandvars(command_line))

                if full_command.startswith("shell:"):
                    # this name matches the C library #define name
                    # noinspection PyPep8Naming
                    SW_SHOWNORMAL = 1
                    ctypes.windll.shell32.ShellExecuteW(None, "open",
                                                        full_command,
                                                        None, None, SW_SHOWNORMAL)
                else:
                    # Use shlex to split the command line into a list of arguments,
                    # correctly handling spaces and quotes.
                    args = shlex.split(full_command)

                    # Popen is non-blocking, so the launcher GUI remains responsive.
                    subprocess.Popen(args)

            elif startfile:
                # on windows, some programs don't start properly unless they go through
                # startfile(). MS Excel is one such program -- for some reason, it won't
                # save its window size&position on exit if it was started by Popen,
                # but it works properly if started by startfile().
                def startthread():
                    # also, start it in a child thread, because this often blocks for a long
                    # time while the application launches (!!)
                    os.startfile(startfile)
                threading.Thread(target=startthread, daemon=True).start()

            if self.minimize_on_launch:
                self.iconify()  # Minimizes the window
        except FileNotFoundError:
            messagebox.showerror("Error", f"program not found:\n{path}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to launch application:\n{e}")

        if self.lower_flag == Lower.IMMEDIATE:
            self.lower()
        elif self.lower_flag == Lower.TIMER:
            def lower_window():
                self.lower()
            self.after(1000, lower_window)

if __name__ == "__main__":
    # Create the main application instance and run it
    if len(sys.argv) != 2:
        print('usage:  PfkAppLauncher.py INI_FILE')
        exit(1)
    app = AppLauncher(config_path=sys.argv[1])
    if app.winfo_exists():
        app.mainloop()
