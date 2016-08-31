
#include "libpfkscriptutil.h"

using namespace std;

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
