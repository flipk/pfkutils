/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __pfkscriptlib_h__
#define __pfkscriptlib_h__ 1

#include <string>

class unix_dgram_socket {
    std::string path;
    int fd;
    void init_common(bool new_path);
    static int counter;
    bool isOk;
public:
    unix_dgram_socket(const std::string &_path)
        : path(_path) { init_common(false); }
    unix_dgram_socket(void) { init_common(true); }
    ~unix_dgram_socket(void);
    static const int MAX_MSG_LEN = 16384;
    const bool ok(void) const { return isOk; }
    const std::string &getPath(void) { return path; }
    int getFd(void) { return fd; }
    void connect(const std::string &remote_path);
    bool send(const std::string &msg);
    bool recv(std::string &msg);
    bool send(const std::string &msg, const std::string &remote_path);
    bool recv(      std::string &msg,       std::string &remote_path);
};


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
    bool zipping; // indicates oldpath is temp and might be replaced
    char newpath[1024]; // [0] == 0 when not open
};
struct pfkscript_msg {
    pfkscript_msg_type type;
    union {
        resp_get_file_path_t resp_get_file_path;
        resp_rollover_now_t resp_rollover_now;
    } u;
};

class pfkscript_ctrl {
    bool isOk;
    unix_dgram_socket sock;
public:
    static const char * env_var_name;
    pfkscript_ctrl(void);
    ~pfkscript_ctrl(void);
    const bool ok(void) const { return isOk; }
    // return false if fail
    bool getFile(std::string &path);
    bool rolloverNow(std::string &oldpath, bool &zipping, std::string &newpath);
    bool closeNow(std::string &oldpath, bool &zipping);
    bool openNow(std::string &newpath);
};


#endif /* __pfkscriptlib_h__ */
