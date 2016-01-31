/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __BACKUP_BAKFILE_H__
#define __BACKUP_BAKFILE_H__

#include "Btree.h"
#include "options.h"

class bakDatum;

class bakFile {
    static const int CACHE_SIZE = 200 * 1024 * 1024;
    static const int BTREE_ORDER = 25;
    Btree * bt;
    FileBlockInterface * fbi;
    const bkOptions &opts;
    bool calc_file_hash(std::string &hash, const std::string &path);
    // returns the hash
    bool put_file(std::string &hash, const std::string &path,
                  const uint64_t filesize);
    void delete_version(int version);
    void extract_file(uint32_t version, const std::string &path);
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
