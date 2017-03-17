/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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

#ifndef __pfkscriptlib_h__
#define __pfkscriptlib_h__ 1

#include <string>
#include "posix_fe.h"

enum pfkscript_msg_type {
    PFKSCRIPT_CMD_GET_FILE_PATH      =   1, // no args
    PFKSCRIPT_RESP_GET_FILE_PATH     = 101, // resp_get_file_path
    PFKSCRIPT_CMD_ROLLOVER_NOW       =   2, // no args
    PFKSCRIPT_RESP_ROLLOVER_NOW      = 102, // resp_rollover_now
    PFKSCRIPT_CMD_CLOSE_NOW          =   3, // no args
    PFKSCRIPT_RESP_CLOSE_NOW         = 103, // resp_rollover_now
    PFKSCRIPT_CMD_OPEN_NOW           =   4, // no args
    PFKSCRIPT_RESP_OPEN_NOW          = 104, // resp_get_file_path
};

struct resp_get_file_path_t {
    char path[1024]; // [0] == 0 when not open
};
struct resp_rollover_now_t {
    char oldpath[1024]; // [0] == 0 when not open
    bool zipped; // indicates oldpath was compressed
    char newpath[1024]; // [0] == 0 when not open
};

struct pfkscript_msg {
    pfkscript_msg_type type;
    union {
        resp_get_file_path_t resp_get_file_path;
        resp_rollover_now_t resp_rollover_now;
    } u;
};
struct PfkscriptMsg {
    std::string  buf;
    pfkscript_msg &m() { return *((pfkscript_msg *)buf.c_str()); }
    PfkscriptMsg(void) { buf.resize(sizeof(pfkscript_msg)); }
};

class pfkscript_ctrl {
    bool isOk;
    pxfe_unix_dgram_socket sock;
public:
    static const char * env_var_name;
    pfkscript_ctrl(void);
    ~pfkscript_ctrl(void);
    const bool ok(void) const { return isOk; }
    // return false if fail
    bool getFile(std::string &path);
    bool rolloverNow(std::string &oldpath, bool &zipped, std::string &newpath);
    bool closeNow(std::string &oldpath, bool &zipped);
    bool openNow(std::string &newpath);
};


#endif /* __pfkscriptlib_h__ */
