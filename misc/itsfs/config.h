
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

/* do_mount.c */

/* should we perform the "mount" command, or should we
   instead manually inject the first getattr command?
   see comments in do_mount.c to see how this works. */
#define DO_MOUNT

/* NFS mounting parameters */
#define FS_TYPE "nfs"
#define FS_DIR "/i"
#define FS_FLAGS (MNT_NOSUID | MNT_NOATIME | MNT_NODEV)

/* svr.C */

/* what port should remote inode worker slaves connect to? */
#define SLAVE_PORT 2700
/* what port should be used for NFS requests to and from the kernel? */
#define SERVER_PORT 2701

/* should we use encryption on the packets? */
#undef  USE_CRYPT
/* should we srandom() based on time and pid? */
#define REALLY_RANDOM
/* should we print all NFS packets sent and received to tty? */
#undef  LOG_PACKETS

/* id_name_db.H */

#define CACHESIZE 256

