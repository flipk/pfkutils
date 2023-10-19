
#if 0
gcc -mno-cygwin -Wl,--subsystem,windows runemacs.c -o runemacs.exe
exit 0
#endif

#include <windows.h>
#include <string.h>
#include <malloc.h>

#define PATH "/usr/local/bin:/usr/bin:/bin:/usr/X11R6/bin:/cygdrive/c/Windows/system32:/cygdrive/c/Windows:/cygdrive/c/Windows/System32"

#define      PROGRAM  "c:\\cygwin\\util\\emacs-22.1\\bin\\emacs.exe"
#define PROGRAM_ARGS  "-fg yellow -bg black"
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

    sprintf( ncmdline, "%s %s %s", PROGRAM, PROGRAM_ARGS, cmdline );
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
