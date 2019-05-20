
4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2

The algorithms (flame, pyro, rotor, swarm, worm) were ported to C++
without any substantial alterations to the algorithms themselves.

The init and shutdown sequences in main.cc were adapted with some
modifications from the original xlock.c.

The remainder of XLOCK2 is original work by pfk@pfk.org.



history below this line is the original xlock documentation.
---------------------------------------------------------------

xlock.c - X11 client to lock a display and show a screen saver.

Copyright (c) 1988-91 by Patrick J. Naughton.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.

This file is provided AS IS with no warranties of any kind.  The author
shall have no liability with respect to the infringement of copyrights,
trade secrets or any patents by this file or any part thereof.  In no
event will the author be liable for any lost revenue or profits or
other special, indirect and consequential damages.

OLD INFO: Comments and additions should be sent to the author:
OLD INFO:
OLD INFO:             naughton@eng.sun.com
OLD INFO:
OLD INFO:             Patrick J. Naughton
OLD INFO:             MS 21-14
OLD INFO:             Sun Laboritories, Inc.
OLD INFO:             2550 Garcia Ave
OLD INFO:             Mountain View, CA  94043

Revision History:
24-Jun-91: make foreground and background color get used on mono.
24-May-91: added -usefirst.
16-May-91: added pyro and random modes.
       ripped big comment block out of all other files.
08-Jan-91: fix some problems with password entry.
       removed renicing code.
29-Oct-90: added cast to XFree() arg.
       added volume arg to call to XBell().
28-Oct-90: center prompt screen.
       make sure Xlib input buffer does not use up all of swap.
       make displayed text come from resource file for better I18N.
       add backward compatible signal handlers for pre 4.1 machines.
31-Aug-90: added blank mode.
       added swarm mode.
       moved usleep() and seconds() out to usleep.c.
       added SVR4 defines to xlock.h
29-Jul-90: added support for multiple screens to be locked by one xlock.
       moved global defines to xlock.h
       removed use of allowsig().
07-Jul-90: reworked commandline args and resources to use Xrm.
       moved resource processing out to resource.c
02-Jul-90: reworked colors to not use dynamic colormap.
23-May-90: added autoraise when obscured.
15-Apr-90: added hostent alias searching for host authentication.
18-Feb-90: added SunOS3.5 fix.
       changed -mono -> -color, and -saver -> -lock.
       allow non-locking screensavers to display on remote machine.
       added -echokeys to disable echoing of '?'s on input.
       cleaned up all of the parameters and defaults.
20-Dec-89: added -xhost to allow access control list to be left alone.
       added -screensaver (don't disable screen saver) for the paranoid.
       Moved seconds() here from all of the display mode source files.
       Fixed bug with calling XUngrabHosts() in finish().
19-Dec-89: Fixed bug in GrabPointer.
       Changed fontname to XLFD style.
23-Sep-89: Added fix to allow local hostname:0 as a display.
       Put empty case for Enter/Leave events.
       Moved colormap installation later in startup.
20-Sep-89: Linted and made -saver mode grab the keyboard and mouse.
       Replaced SunView code for life mode with Jim Graham's version,
     so I could contrib it without legal problems.
       Sent to expo for X11R4 contrib.
19-Sep-89: Added '?'s on input.
27-Mar-89: Added -qix mode.
       Fixed GContext->GC.
20-Mar-89: Added backup font (fixed) if XQueryLoadFont() fails.
       Changed default font to lucida-sans-24.
08-Mar-89: Added -nice, -mode and -display, built vector for life and hop.
24-Feb-89: Replaced hopalong display with life display from SunView1.
22-Feb-89: Added fix for color servers with n < 8 planes.
16-Feb-89: Updated calling conventions for XCreateHsbColormap();
       Added -count for number of iterations per color.
       Fixed defaulting mechanism.
       Ripped out VMS hacks.
       Sent to expo for X11R3 contrib.
15-Feb-89: Changed default font to pellucida-sans-18.
20-Jan-89: Added -verbose and fixed usage message.
19-Jan-89: Fixed monochrome gc bug.
16-Dec-88: Added SunView style password prompting.
19-Sep-88: Changed -color to -mono. (default is color on color displays).
       Added -saver option. (just do display... don't lock.)
31-Aug-88: Added -time option.
       Removed code for fractals to separate file for modularity.
       Added signal handler to restore host access.
       Installs dynamic colormap with a Hue Ramp.
       If grabs fail then exit.
       Added VMS Hacks. (password 'iwiwuu').
       Sent to expo for X11R2 contrib.
08-Jun-88: Fixed root password pointer problem and changed PASSLENGTH to 20.
20-May-88: Added -root to allow root to unlock.
12-Apr-88: Added root password override.
       Added screen saver override.
       Removed XGrabServer/XUngrabServer.
       Added access control handling instead.
01-Apr-88: Added XGrabServer/XUngrabServer for more security.
30-Mar-88: Removed startup password requirement.
       Removed cursor to avoid phosphor burn.
27-Mar-88: Rotate fractal by 45 degrees clockwise.
24-Mar-88: Added color support. [-color]
       wrote the man page.
23-Mar-88: Added HOPALONG routines from Scientific American Sept. 86 p. 14.
       added password requirement for invokation
       removed option for command line password
       added requirement for display to be "unix:0".
22-Mar-88: Recieved Walter Milliken's comp.windows.x posting.


