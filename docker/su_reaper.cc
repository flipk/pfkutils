
// NOTE compiling this file is tricky, because the host machine
// where the docker is built is no-good for building this. it has
// to run in the docker environment, which has different libc and
// probably different vdso and libgcc and ld.so and the binary
// compiled on the host may not even work inside the docker.
// so to build this we do:
//     docker run fedora:38
//     dnf install g++
//     g++ su_reaper.cc -o su_reaper
// and the resulting binary is what gets installed in the final
// docker image. kind of gross, but you gotta do what you gotta do.

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <grp.h>

static pid_t our_bash_pid = -1;
static bool run = true;

static void
sighand(int s)
{
    pid_t pid = -1;
    int status;
    if (s == SIGCHLD)
        do {
            pid = waitpid(/*wait for any child*/-1, &status, WNOHANG);
            if (pid > 0)
            {
                if (pid == our_bash_pid)
                {
                    printf("su_reaper: detected death of our bash child "
                           "pid %u  with status %d\n",
                           (uint32_t) pid, status);

                    // we're done
                    run = false;
                }
                else
                {
                    printf("su_reaper: detected death of orphaned child "
                           "pid %u  with status %d\n",
                           (uint32_t) pid, status);
                }
            }
        } while (pid > 0);
}

int main(int argc, char ** argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "usage: su_reaper  uid gid docker_gid cmd\n");
        exit(1);
    }

    int uid = atoi(argv[1]);
    int gid = atoi(argv[2]);
    int docker_gid = atoi(argv[3]);
    const char *cmd = argv[4];

    struct sigaction sa;
    sa.sa_handler = &sighand;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &sa, NULL);

    // set ourselves as a reaper so we get all
    // the zombies and we can reap them and clean up.
    // otherwise zombies take over the world.
    if (prctl(PR_SET_CHILD_SUBREAPER, 1) < 0)
    {
        int e = errno;
        printf("prctl SUBREAPER : %d (%s)\n",
               e, strerror(e));
        // otherwise ignore
    }

    int exec_error_pipe[2];
    pipe(exec_error_pipe);

    // use vfork because all we're going
    // to do is exec a child.
    pid_t pid = vfork();
    our_bash_pid = pid;

    if (pid == 0)
    {
        // child

        // child does not need read end of this pipe.
        close(exec_error_pipe[0]);

        // child will close write-end if exec succeeds.
        fcntl(exec_error_pipe[1], F_SETFD, FD_CLOEXEC);

        // set my process credentials!
        gid_t  mygid;
        if (docker_gid > 0)
        {
            mygid = (gid_t) docker_gid;
            setgroups(1, &mygid);
        }
        mygid = (gid_t) gid;
        setgid(mygid);
        uid_t  myuid = (uid_t) uid;
        setuid(myuid);

        // run the shell.
        (void) execl(cmd, cmd, NULL);

        // if exec is successful, pipe[1] is CLOEXEC'd
        // and we don't get here. if exec is failed, we
        // get here and send errno back to parent.
        int e = errno;
        (void) write(exec_error_pipe[1], &e, sizeof(e));

        // call _exit, not exit, because we don't want
        // to call registered atexit() handlers in a vfork'd
        // child.
        _exit(99);
    }
    // else parent

    // parent doesn't need write-end.
    close(exec_error_pipe[1]);
    int e;
    int cc = read(exec_error_pipe[0], &e, sizeof(e));
    // parent no longer needs read-end.
    close(exec_error_pipe[0]);

    // if cc == 0, the child's exec succeeded.
    // if cc == sizeof(int) then the exec failed and we
    // are getting an errno.
    if (cc == sizeof(e))
    {
        fprintf(stderr, "EXEC FAILED: %d (%s)\n",
                e, strerror(e));
        return 1;
    }

    // no longer anything to do but sit here catching
    // SIGCHLDs and reaping zombies.
    while (run)
    {
        // NOTE 10 seconds sounds like a long time
        // to detect our intended child is dead, but actually
        // the SIGCHLD interrupts this sleep and we detect
        // the run==false instantly.
        sleep(10);
    }

    // we don't have to go collect any more orphans, because we're
    // about to exit, giving up our subreaper status, which reparents
    // all our lost orphans up to init(1).

    // we also don't worry about running children, because this program
    // is intended to be run inside docker, and this exits when docker
    // exits, and docker will clean all that crap up.

    return 0;
}
