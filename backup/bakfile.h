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

#ifndef __BACKUP_BAKFILE_H__
#define __BACKUP_BAKFILE_H__

#include "Btree.h"
#include "options.h"

class bakDatum;

class bakFile {
    static const int CACHE_SIZE_INDEX =  100 * 1024 * 1024;
    static const int CACHE_SIZE_DATA  = 1000 * 1024 * 1024;
    static const int BTREE_ORDER = 25;
    Btree * bt; // the index database
    FileBlockInterface * fbi; // of the btree
    FileBlockInterface * fbi_data; // of the data
    bool openFiles(void);
    bool createFiles(void);
    const bkOptions &opts;
    bool calc_file_hash(std::string &hash, const std::string &path);
    // returns the hash
    bool put_file(std::string &hash, const std::string &path,
                  const uint64_t filesize);
    void delete_version(int version);
    bool extract_file(uint32_t version, const std::string &path, int tarfd);
    void _extract(int tarfd);
    bool match_file_name(const std::string &path);
    void _update(void);
    void create(void);
    void update(void);
    void listdb(void);
    void deletevers(void);
    void extract(void);
    void export_tar(void);
public:
    bakFile(const bkOptions &_opts);
    ~bakFile(void);
    void operate(void);
};

#endif /*__BACKUP_BAKFILE_H__*/
