/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __BACKUP_OPTIONS_H__
#define __BACKUP_OPTIONS_H__

#include <string>
#include <vector>

enum opcode_t {
    OP_NONE,
    OP_CREATE,
    OP_UPDATE,
    OP_LIST,
    OP_DELETE,
    OP_EXTRACT,
    OP_EXPORT
};

class bkOptions {
    void printUsage(void);
    bool _parse(int argc, char ** argv);
public:
    std::string backupfile_index;
    std::string backupfile_data;
    std::string sourcedir; // for create
    opcode_t op;
    std::vector<int> versions; // for delete, or one version for extract/export
    std::vector<std::string> paths; // for extract or export, empty for all
    std::string tarfile; // for export
    int verbose; // 0, 1, or 2
    //
    bkOptions(void);
    // return false if bad cmdline options
    bool parse(int argc, char ** argv);
};

#endif /*__BACKUP_OPTIONS_H__*/
