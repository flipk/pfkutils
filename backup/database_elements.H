
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __DATABASE_ELEMENTS_H__
#define __DATABASE_ELEMENTS_H__

#include "bst.H"
#include "Btree.H"
#include "BtreeDbClasses.H"
#include "pk-md5.h"

#define TOOL_VERSION 1
#define INFO_KEY  "PfkBakDbInfo"

struct PfkBakDbKeys : public BST {
    enum Prefix {
        PREFIX_BACKUP_DB_INFO,
        PREFIX_BACKUP_INFO,
        PREFIX_FILE_INFO,
        PREFIX_FILE_PIECE_INFO,
        PREFIX_FILE_PIECE_DATA
    };
    PfkBakDbKeys( Prefix _p ) : BST(NULL), prefix(this) { prefix.v = _p; }
    BST_UINT8_t    prefix;   /**< MUST be included in BSG_FIELD_LIST of
                                any derived type. */
};

struct PfkBackupDbInfoKey : public PfkBakDbKeys {
    PfkBackupDbInfoKey(void) :
        PfkBakDbKeys(PREFIX_BACKUP_DB_INFO),
        info_key(this) { }
    BST_STRING  info_key;
    // no additional data required for this key
};

struct PfkBackupDbInfoData : public FileBlockBST {
    PfkBackupDbInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        tool_version(this), backups(this) { }
    ~PfkBackupDbInfoData(void) { bst_free(); }
    BST_UINT32_t              tool_version;
    BST_ARRAY <BST_UINT32_t>  backups;
};

DB_ITEM_CLASS(PfkBackupDbInfo);

struct PfkBackupInfoKey : public PfkBakDbKeys {
    PfkBackupInfoKey(void) :
        PfkBakDbKeys(PREFIX_BACKUP_INFO),
        backup_number(this) {}
    BST_UINT32_t   backup_number;
};

struct PfkBakGenInfo : public BST {
    PfkBakGenInfo(BST *parent) :
        BST(parent),
        date_time(this), generation_number(this) { }
    BST_STRING    date_time;
    BST_UINT32_t  generation_number;
};

struct PfkBackupInfoData : public FileBlockBST {
    PfkBackupInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        root_dir(this), name(this), comment(this),
        next_generation_number(this), generations(this) {}
    ~PfkBackupInfoData(void) { bst_free(); }
    BST_STRING    root_dir;
    BST_STRING    name;
    BST_STRING    comment;
    BST_UINT32_t  next_generation_number;
    BST_ARRAY <PfkBakGenInfo> generations;
};

DB_ITEM_CLASS(PfkBackupInfo);

struct PfkBackupFileInfoKey : public PfkBakDbKeys {
    PfkBackupFileInfoKey(void) :
        PfkBakDbKeys(PREFIX_FILE_INFO),
        backup_number(this), file_number(this) { }
    BST_UINT32_t   backup_number;
    BST_UINT32_t   file_number;
};

struct PfkBackupFileInfoData : public FileBlockBST {
    PfkBackupFileInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        file_path(this), size(this), mtime(this) { }
    ~PfkBackupFileInfoData(void) { bst_free(); }
    BST_STRING     file_path;
    BST_UINT64_t   size;
    BST_UINT32_t   mtime;
};

DB_ITEM_CLASS(PfkBackupFileInfo);

struct PfkBackupFilePieceInfoKey : public PfkBakDbKeys {
    PfkBackupFilePieceInfoKey(void) :
        PfkBakDbKeys(PREFIX_FILE_PIECE_INFO),
        backup_number(this), file_number(this), piece_number(this) { }
    BST_UINT32_t   backup_number;
    BST_UINT32_t   file_number;
    BST_UINT32_t   piece_number;
};

struct PfkBackupVersion : public BST {
    PfkBackupVersion(BST *parent) :
        BST(parent),
        gen_number(this), md5hash(this) { }
    BST_UINT32_t    gen_number;
    BST_FIXED_BINARY <MD5_DIGEST_SIZE>  md5hash;
};

struct PfkBackupFilePieceInfoData : public FileBlockBST {
    PfkBackupFilePieceInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        versions(this) { }
    ~PfkBackupFilePieceInfoData(void) { bst_free(); }
    BST_ARRAY <PfkBackupVersion> versions;
};

DB_ITEM_CLASS(PfkBackupFilePieceInfo);

struct PfkBackupFilePieceDataKey : public PfkBakDbKeys {
    PfkBackupFilePieceDataKey(void) : 
        PfkBakDbKeys(PREFIX_FILE_PIECE_DATA),
        backup_number(this), file_number(this),
        piece_number(this), md5hash(this) { }
    BST_UINT32_t    backup_number;
    BST_UINT32_t    file_number;
    BST_UINT32_t    piece_number;
    BST_FIXED_BINARY <MD5_DIGEST_SIZE>  md5hash;
};

struct PfkBackupFilePieceDataData : public FileBlockBST {
    PfkBackupFilePieceDataData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        refcount(this), data_fbn(this) { }
    ~PfkBackupFilePieceDataData(void) { bst_free(); }
    BST_UINT32_t   refcount;
    BST_UINT32_t   data_fbn;
};

DB_ITEM_CLASS(PfkBackupFilePieceData);

#endif /* __DATABASE_ELEMENTS_H__ */
