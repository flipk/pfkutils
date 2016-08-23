
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

void
unix_dgram_socket :: init_common(bool new_path)
{
    isOk = false;
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
        return;
    }
    struct sockaddr_un sa;
    sa.sun_family = AF_UNIX;
    int len = sizeof(sa.sun_path)-1;
    strncpy(sa.sun_path, path.c_str(), len);
    sa.sun_path[len] = 0;
    if (::bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        int e = errno;
        cerr << "bind: " << strerror(e) << endl;
        ::close(fd);
        fd = -1;
        return;
    }
    isOk = true;
}

unix_dgram_socket :: ~unix_dgram_socket(void)
{
    if (fd > 0)
    {
        ::close(fd);
        (void) unlink( path.c_str() );
    }
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

pfkscript_ctrl::pfkscript_ctrl(void)
{
    isOk = false;
    char * ctrl_path = getenv("PFKSCRIPT_CTRL_SOCKET");
    if (ctrl_path == NULL)
    {
        cerr << "cannot open control socket: env var not set" << endl;
        return;
    }
    sock.connect(ctrl_path);
    isOk = true;
}

pfkscript_ctrl::~pfkscript_ctrl(void)
{
}

// return false if failure
bool
pfkscript_ctrl::getFile(std::string &path, bool &isopen)
{
    string _msg;
    pfkscript_msg * msg;

    _msg.resize(sizeof(pfkscript_msg));
    msg = (pfkscript_msg *) _msg.c_str();
    msg->type = PFKSCRIPT_CMD_GET_FILE_PATH;
    sock.send(_msg);
    if (sock.recv(_msg))
    {
        msg = (pfkscript_msg *) _msg.c_str();
        if (msg->type == PFKSCRIPT_CMD_GET_FILE_PATH_RESP)
        {
            if (msg->u.cmd_get_file_path_resp.open == false)
            {
                isopen = false;
                return true;
            }
            isopen = true;
            path.assign(msg->u.cmd_get_file_path_resp.path);
            return true;
        }
        return false;
    }
    return false;
}
