
#include "libpfkscriptutil.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include <iostream>
#include <sstream>

using namespace std;

//static
int unix_dgram_socket :: counter = 1;

unix_dgram_socket :: unix_dgram_socket(void)
{
    path.clear();
    fd = -1;
}

unix_dgram_socket :: ~unix_dgram_socket(void)
{
    if (fd > 0)
    {
        ::close(fd);
        (void) unlink( path.c_str() );
    }
}

bool
unix_dgram_socket :: init_common(bool new_path)
{
    if (new_path)
    {
        const char * temp_path = getenv("TMP");
        if (temp_path == NULL)
            temp_path = getenv("TEMP");
        if (temp_path == NULL)
            temp_path = getenv("TMPDIR");
        if (temp_path == NULL)
            temp_path = "/tmp";
        ostringstream  str;
        str << temp_path << "/udslibtmp." << getpid() << "." << counter;
        counter++;
        path = str.str();
    }
    fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        int e = errno;
        cerr << "socket: " << strerror(e) << endl;
        return false;
    }
    struct sockaddr_un sa;
    sa.sun_family = AF_UNIX;
    int len = sizeof(sa.sun_path)-1;
    strncpy(sa.sun_path, path.c_str(), len);
    sa.sun_path[len] = 0;
    if (::bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        int e = errno;
        cerr << "bind dgram: " << strerror(e) << endl;
        ::close(fd);
        fd = -1;
        return false;
    }
    return true;
}

void
unix_dgram_socket :: connect(const std::string &remote_path)
{
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    int len = sizeof(addr.sun_path)-1;
    strncpy(addr.sun_path, remote_path.c_str(), len);
    addr.sun_path[len] = 0;
    if (::connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        int e = errno;
        cerr << "connect: " << e << ": " << strerror(e) << endl;
    }
}

bool
unix_dgram_socket :: send(const std::string &msg)
{
    if (msg.size() > MAX_MSG_LEN)
    {
        cerr << "ERROR unix_dgram_socket send msg size of "
             << msg.size() << " is greater than max "
             << MAX_MSG_LEN << endl;
        return false;
    }
    if (::send(fd, msg.c_str(), msg.size(), /*flags*/0) < 0)
    {
        int e = errno;
        cerr << "send: " << e << ": " << strerror(e) << endl;
        return false;
    }
    return true;

}

bool
unix_dgram_socket :: recv(std::string &msg)
{
    msg.resize(MAX_MSG_LEN);
    ssize_t msglen = ::recv(fd, (void*) msg.c_str(), MAX_MSG_LEN, /*flags*/0);
    if (msglen <= 0)
    {
        int e = errno;
        cerr << "recv: " << e << ": " << strerror(e) << endl;
        msg.resize(0);
        return false;
    }
    msg.resize(msglen);
    return true;
}

bool
unix_dgram_socket :: send(const std::string &msg,
                          const std::string &remote_path)
{
    if (msg.size() > MAX_MSG_LEN)
    {
        cerr << "ERROR unix_dgram_socket send msg size of "
             << msg.size() << " is greater than max "
             << MAX_MSG_LEN << endl;
        return false;
    }
    struct sockaddr_un sa;
    sa.sun_family = AF_UNIX;
    int len = sizeof(sa.sun_path)-1;
    strncpy(sa.sun_path, remote_path.c_str(), len);
    sa.sun_path[len] = 0;
    if (::sendto(fd, msg.c_str(), msg.size(), /*flags*/0,
               (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        int e = errno;
        cerr << "sendto: " << e << ": " << strerror(e) << endl;
        return false;
    }
    return true;
}

bool
unix_dgram_socket :: recv(std::string &msg,
                          std::string &remote_path)
{
    msg.resize(MAX_MSG_LEN);
    struct sockaddr_un sa;
    socklen_t salen = sizeof(sa);
    ssize_t msglen = ::recvfrom(fd, (void*) msg.c_str(), MAX_MSG_LEN,
                              /*flags*/0, (struct sockaddr *)&sa, &salen);
    if (msglen <= 0)
    {
        int e = errno;
        cerr << "recvfrom: " << e << ": " << strerror(e) << endl;
        msg.resize(0);
        return false;
    }
    msg.resize(msglen);
    remote_path.assign(sa.sun_path);
    return true;
}

//////////////////////////////////////////////////////////////////////////////

//static
const char * pfkscript_ctrl::env_var_name = "PFKSCRIPT_CTRL_SOCKET";

pfkscript_ctrl::pfkscript_ctrl(void)
{
    isOk = false;
    char * ctrl_path = getenv(env_var_name);
    if (ctrl_path == NULL)
    {
        cerr << "cannot open control socket: env var not set" << endl;
        return;
    }
    if (sock.init() == true)
    {
        sock.connect(ctrl_path);
        isOk = true;
    }
}

pfkscript_ctrl::~pfkscript_ctrl(void)
{
}

// return false if failure
bool
pfkscript_ctrl::getFile(std::string &path)
{
    PfkscriptMsg   m;
    m.m().type = PFKSCRIPT_CMD_GET_FILE_PATH;
    sock.send(m.buf);
    if (sock.recv(m.buf))
    {
        if (m.m().type == PFKSCRIPT_RESP_GET_FILE_PATH)
        {
            resp_get_file_path_t * resp = &m.m().u.resp_get_file_path;
            path.assign(resp->path);
            return true;
        }
    }
    return false;
}

bool
pfkscript_ctrl::rolloverNow(std::string &oldpath,
                            bool &zipped,
                            std::string &newpath)
{
    PfkscriptMsg  m;
    m.m().type = PFKSCRIPT_CMD_ROLLOVER_NOW;
    sock.send(m.buf);
    if (sock.recv(m.buf))
    {
        if (m.m().type == PFKSCRIPT_RESP_ROLLOVER_NOW)
        {
            resp_rollover_now_t * resp = &m.m().u.resp_rollover_now;
            oldpath.assign(resp->oldpath);
            zipped = resp->zipped;
            newpath.assign(resp->newpath);
            return true;
        }
    }
    return false;
}

bool
pfkscript_ctrl::closeNow(std::string &oldpath, bool &zipped)
{
    PfkscriptMsg m;
    m.m().type = PFKSCRIPT_CMD_CLOSE_NOW;
    sock.send(m.buf);
    if (sock.recv(m.buf))
    {
        if (m.m().type == PFKSCRIPT_RESP_CLOSE_NOW)
        {
            resp_rollover_now_t * resp = &m.m().u.resp_rollover_now;
            oldpath.assign(resp->oldpath);
            zipped = resp->zipped;
            return true;
        }
    }
    return false;
}

bool
pfkscript_ctrl::openNow(std::string &newpath)
{
    PfkscriptMsg m;
    m.m().type = PFKSCRIPT_CMD_OPEN_NOW;
    sock.send(m.buf);
    if (sock.recv(m.buf))
    {
        if (m.m().type == PFKSCRIPT_RESP_OPEN_NOW)
        {
            resp_get_file_path_t * resp = &m.m().u.resp_get_file_path;
            newpath.assign(resp->path);
            return true;
        }
    }
    return false;
}
