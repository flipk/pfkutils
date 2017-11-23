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

#ifndef __BACKUP_OPTIONS_H__
#define __BACKUP_OPTIONS_H__

#include <string>
#include <vector>

#define SINGLE_FILE_BACKUP 0

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
#if SINGLE_FILE_BACKUP == 0
    std::string backupfile_index;
#endif
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
