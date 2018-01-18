/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

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
        pxfe_errno e;
        if (sock.connect(ctrl_path, &e) == false)
            cerr << e.Format() << endl;
        else
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
    pxfe_errno e;
    m.m().type = PFKSCRIPT_CMD_GET_FILE_PATH;
    if (sock.send(m.buf, &e) == false)
    {
        cerr << e.Format() << endl;
        return false;
    }
    if (sock.recv(m.buf, &e))
    {
        if (m.m().type == PFKSCRIPT_RESP_GET_FILE_PATH)
        {
            resp_get_file_path_t * resp = &m.m().u.resp_get_file_path;
            path.assign(resp->path);
            return true;
        }
    }
    else
    {
        cerr << e.Format() << endl;
    }
    return false;
}

bool
pfkscript_ctrl::rolloverNow(std::string &oldpath,
                            bool &zipped,
                            std::string &newpath)
{
    PfkscriptMsg  m;
    pxfe_errno e;
    m.m().type = PFKSCRIPT_CMD_ROLLOVER_NOW;
    if (sock.send(m.buf, &e) == false)
    {
        cerr << e.Format() << endl;
        return false;
    }
    if (sock.recv(m.buf, &e))
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
    else
    {
        cerr << e.Format() << endl;
    }
    return false;
}

bool
pfkscript_ctrl::closeNow(std::string &oldpath, bool &zipped)
{
    PfkscriptMsg m;
    pxfe_errno e;
    m.m().type = PFKSCRIPT_CMD_CLOSE_NOW;
    if (sock.send(m.buf, &e) == false)
    {
        cerr << e.Format() << endl;
        return false;
    }
    if (sock.recv(m.buf, &e))
    {
        if (m.m().type == PFKSCRIPT_RESP_CLOSE_NOW)
        {
            resp_rollover_now_t * resp = &m.m().u.resp_rollover_now;
            oldpath.assign(resp->oldpath);
            zipped = resp->zipped;
            return true;
        }
    }
    else
    {
        cerr << e.Format() << endl;
    }
    return false;
}

bool
pfkscript_ctrl::openNow(std::string &newpath)
{
    PfkscriptMsg m;
    pxfe_errno e;
    m.m().type = PFKSCRIPT_CMD_OPEN_NOW;
    if (sock.send(m.buf, &e) == false)
    {
        cerr << e.Format() << endl;
        return false;
    }
    if (sock.recv(m.buf, &e))
    {
        if (m.m().type == PFKSCRIPT_RESP_OPEN_NOW)
        {
            resp_get_file_path_t * resp = &m.m().u.resp_get_file_path;
            newpath.assign(resp->path);
            return true;
        }
    }
    else
    {
        cerr << e.Format() << endl;
    }
    return false;
}
