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
    static const int MAX_MSG_LEN = 1500;
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
    PFKSCRIPT_CMD_GET_FILE_PATH      =   1,
    PFKSCRIPT_CMD_GET_FILE_PATH_RESP = 101,
};

struct pfkscript_msg {
    pfkscript_msg_type type;
    union {
        struct {
            bool open;
            char path[1024];
        } cmd_get_file_path_resp;
    } u;
};

#define PFKSCRIPT_ENV_VAR_NAME "PFKSCRIPT_CTRL_SOCKET"

class pfkscript_ctrl {
    bool isOk;
    unix_dgram_socket sock;
public:
    pfkscript_ctrl(void);
    ~pfkscript_ctrl(void);
    const bool ok(void) const { return isOk; }
    // return false if file not currently open.
    bool getFile(std::string &path, bool &isopen);
};


#endif /* __pfkscriptlib_h__ */
