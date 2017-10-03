/*
  Simple program to start Emacs with its console window hidden.

  This program is provided purely for convenience, since most users will
  use Emacs in windowing (GUI) mode, and will not want to have an extra
  console window lying around.  */

/*
   You may want to define this if you want to be able to install updated
   emacs binaries even when other users are using the current version.
   The problem with some file servers (notably Novell) is that an open
   file cannot be overwritten, deleted, or even renamed.  So if someone
   is running emacs.exe already, you cannot install a newer version.
   By defining CHOOSE_NEWEST_EXE, you can name your new emacs.exe
   something else which matches "emacs*.exe", and runemacs will
   automatically select the newest emacs executeable in the bin directory.
   (So you'll probably be able to delete the old version some hours/days
   later).
*/

/* #define CHOOSE_NEWEST_EXE */

#define WIN32

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
  int wait_for_child = FALSE;
  DWORD priority_class = NORMAL_PRIORITY_CLASS;
  DWORD ret_code = 0;
  char new_cmdline[200];
  char *p;
  char modname[MAX_PATH];

  if (!GetModuleFileName (NULL, modname, MAX_PATH))
    goto error;
  if ((p = strrchr (modname, '\\')) == NULL)
    goto error;
  *p = 0;

  strcpy (new_cmdline, "c:\\cygwin\\util\\emacs-20.7\\bin\\emacs.exe");

  SetEnvironmentVariable ("emacs_dir", "c:\\cygwin\\util\\emacs-20.7");
  SetEnvironmentVariable ("HOME", "c:\\Documents and Settings\\pknaack1");

  memset (&start, 0, sizeof (start));
  start.cb = sizeof (start);
  start.dwFlags = STARTF_USESHOWWINDOW;
  start.wShowWindow = SW_HIDE;

  sec_attrs.nLength = sizeof (sec_attrs);
  sec_attrs.lpSecurityDescriptor = NULL;
  sec_attrs.bInheritHandle = FALSE;

  if (CreateProcess (NULL, new_cmdline, &sec_attrs, NULL, TRUE, priority_class,
		     GetEnvironmentStrings (), NULL, &start, &child))
    {
      if (wait_for_child)
	{
	  WaitForSingleObject (child.hProcess, INFINITE);
	  GetExitCodeProcess (child.hProcess, &ret_code);
	}
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
