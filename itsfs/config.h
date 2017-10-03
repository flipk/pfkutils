/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* do_mount.c */

/* should we perform the "mount" command, or should we
   instead manually inject the first getattr command?
   see comments in do_mount.c to see how this works. */
#define DO_MOUNT

/* NFS mounting parameters */
#define FS_TYPE "nfs"
#define FS_DIR "/i"
#define FS_FLAGS 0

/* svr.C */

/* what port should remote inode worker slaves connect to? */
#define SLAVE_PORT 2700
/* what port should be used for NFS requests to and from the kernel? */
#define SERVER_PORT 2701

/* should we use encryption on the packets? */
#define USE_CRYPT
/* should we srandom() based on time and pid? */
#define REALLY_RANDOM
/* should we print all NFS packets sent and received to tty? */
#undef  LOG_PACKETS

/* id_name_db.H */

#define CACHESIZE 64,16,256

