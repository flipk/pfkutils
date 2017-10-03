
/*
gcc -mno-cygwin -Wl,--subsystem,windows runemacs.c -o r
*/

#include <windows.h>
#include <string.h>
#include <malloc.h>

int WINAPI
WinMain (HINSTANCE hSelf, HINSTANCE hPrev, LPSTR cmdline, int nShow)
{
    STARTUPINFO start;
    SECURITY_ATTRIBUTES sec_attrs;
    SECURITY_DESCRIPTOR sec_desc;
    PROCESS_INFORMATION child;
    DWORD ret_code = 0;
    char ncmdline[MAX_PATH];

    strcpy( ncmdline,
            "c:\\cygwin\\util\\emacs-20.7\\bin\\emacs.exe "
            "-bg black -fg yellow" );
    SetEnvironmentVariable( "emacs_dir",
                            "c:\\cygwin\\util\\emacs-20.7\\bin" );
    SetEnvironmentVariable( "HOME",
                            "c:\\Documents and Settings\\PKNAACK1" );
    chdir( "c:\\Documents and Settings\\PKNAACK1" );

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
