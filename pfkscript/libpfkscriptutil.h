/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __pfkscriptlib_h__
#define __pfkscriptlib_h__ 1

#include <string>
#include "pfkposix.h"

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
    pfk_unix_dgram_socket sock;
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
