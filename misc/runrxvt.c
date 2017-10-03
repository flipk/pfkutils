
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if 0
gcc -mno-cygwin -Wl,--subsystem,windows runrxvt.c -DGEOM=1 -o runrxvt15.exe
gcc -mno-cygwin -Wl,--subsystem,windows runrxvt.c -DGEOM=2 -o runrxvt20.exe
exit 0
#endif

#include <windows.h>
#include <string.h>
#include <malloc.h>

#define PATH "/usr/local/bin:/usr/bin:/bin:/usr/X11R6/bin:/cygdrive/c/Windows/system32:/cygdrive/c/Windows:/cygdrive/c/Windows/System32/Wbem:/cygdrive/c/Program Files/QuickTime/QTSystem/:/usr/lib/lapack"

#if GEOM==1
#define GEOMETRY "10x15"
#else
#define GEOMETRY "10x20"
#endif

#define      PROGRAM  "c:\\cygwin\\bin\\rxvt.exe"
#define PROGRAM_ARGS  "-font " GEOMETRY " -e bash -i"
#define         HOME  "c:\\Users\\flipk"

int WINAPI
WinMain (HINSTANCE hSelf, HINSTANCE hPrev, LPSTR cmdline, int nShow)
{
    STARTUPINFO start;
    SECURITY_ATTRIBUTES sec_attrs;
    SECURITY_DESCRIPTOR sec_desc;
    PROCESS_INFORMATION child;
    DWORD ret_code = 0;
    char ncmdline[MAX_PATH];

    sprintf(ncmdline, "%s %s %s", PROGRAM, PROGRAM_ARGS, cmdline);
    SetEnvironmentVariable("PATH", PATH);
    SetEnvironmentVariable("HOME", HOME);
    SetCurrentDirectory(HOME);

    memset (&start, 0, sizeof (start));
    start.cb = sizeof (start);
    start.dwFlags = STARTF_USESHOWWINDOW;
    start.wShowWindow = SW_HIDE;

    sec_attrs.nLength = sizeof (sec_attrs);
    sec_attrs.lpSecurityDescriptor = NULL;
    sec_attrs.bInheritHandle = FALSE;

    if (CreateProcess (NULL, ncmdline, &sec_attrs, NULL, TRUE,
                       NORMAL_PRIORITY_CLASS, GetEnvironmentStrings (),
                       NULL, &start, &child))
    {
        CloseHandle (child.hThread);
        CloseHandle (child.hProcess);
    }
    else
        goto error;

    return (int) ret_code;

 error:
    MessageBox (NULL, "Could not start Emacs.", "Error", MB_ICONSTOP);
    return 1;
}
