#include <inttypes.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>

// write data to an O_NONBLOCK socket; handle partial writes,
// completing them when possible; timeout if data can't be written.
// return true if ok, false if failure.
bool do_write(int fd, const uint8_t * data, int len, int timeout_s=1)
{
    fd_set wfds;
    struct timeval tv;

    tv.tv_sec = timeout_s;
    tv.tv_usec = 0;
    while (len > 0)
    {
        int got = ::write(fd, data, len);
        if (got == 0)
            // end of the line! not a common case, but handle it anyway.
            return false;
        if (got < 0)
        {
            if (errno == EWOULDBLOCK)
            {
                FD_ZERO(&wfds);
                FD_SET(fd, &wfds);
                if (::select(fd+1, NULL, &wfds, NULL, &tv) <= 0)
                    // timed out! (or error, either way...)
                    return false;
            }
            else
            {
                if (errno != EINTR)
                    // some other error
                    return false;
            }
        }
        else // got > 0
        {
            data += got;
            len -= got;
        }
    }
    // wrote all the data, done!
    return true;
}
