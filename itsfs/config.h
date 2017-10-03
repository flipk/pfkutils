/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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

