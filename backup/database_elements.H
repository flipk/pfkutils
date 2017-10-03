
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

/** \file database_elements.H
 * \brief Defines all data units which appear in a backup database.
 * \author Phillip F Knaack
 */

#ifndef __DATABASE_ELEMENTS_H__
#define __DATABASE_ELEMENTS_H__

#include "bst.H"
#include "Btree.H"
#include "BtreeDbClasses.H"
#include "pk-md5.h"

/** every time the database structure layout changes, 
    the tool version should be incremented! */
#define PFK_BACKUP_TOOL_VERSION 2

/** base type for database keys used in the backup database.
 * This type makes it easier to encapsulate a type field using
 * an enum. All derived classes must call the base class constructor
 * identifying the type of class.
 */
struct PfkBakDbKeys : public BST {
    enum Prefix {
        PREFIX_NONE,             /**< 0 is invalid for a prefix type */
        PREFIX_BACKUP_DB_INFO,   /**< PfkBackupDbInfo */
        PREFIX_BACKUP_INFO,      /**< PfkBackupInfo */
        PREFIX_FILE_INFO,        /**< PfkBackupFileInfo */
        PREFIX_FILE_PIECE_INFO,  /**< PfkBackupFilePieceInfo */
        PREFIX_FILE_PIECE_DATA   /**< PfkBackupFilePieceData */
    };
    /** base class constructor must be passed the prefix type. */
    PfkBakDbKeys( Prefix _p ) : BST(NULL), prefix(this) { prefix.v = _p; }
    BST_UINT8_t    prefix;   /**< MUST be included in BSG_FIELD_LIST of
                                any derived type. */
};

typedef BST_FIXED_BINARY <MD5_DIGEST_SIZE> BST_MD5;

/** key class for the list of backups in the db.
 * This is a singleton (there exists only one of these in the database).
 */
struct PfkBackupDbInfoKey : public PfkBakDbKeys {
    PfkBackupDbInfoKey(void) :
        PfkBakDbKeys(PREFIX_BACKUP_DB_INFO),
        info_key(this) { }
    BST_STRING  info_key;   /**< this should always be INFO_KEY */
    static const char * INFO_KEY;  /**< the constant string "PfkBakDbInfo" */
};

/** data class for the list of backups in the db.
 * This is a singleton (there exists only one of these in the database).
 */
struct PfkBackupDbInfoData : public FileBlockBST {
    PfkBackupDbInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        tool_version(this), backups(this) { }
    ~PfkBackupDbInfoData(void) { bst_free(); }
    /** every time the database structure layout changes, 
        the tool version should be incremented! */
    BST_UINT32_t              tool_version;
    /** a list of all the backup ids used in this database.
        these values are the keys in all other data structures
        which have 'backup_number' in their keys. */ 
    BST_ARRAY <BST_UINT32_t>  backups;
    static const int TOOL_VERSION = PFK_BACKUP_TOOL_VERSION;
};

/** \class PfkBackupDbInfo
 * This is a container for PfkBackupDbInfoKey and PfkBackupDbInfoData,
 * derived from DB_ITEM. */
DB_ITEM_CLASS(PfkBackupDbInfo);

// PfkBackupInfo - one of these for each backup, describing 
//   the backup and number of files and list of generations.

/** key class for information about each backup.
 * the data class lists the root dir, list of generations, comment,
 * next generation number, etc.
 */
struct PfkBackupInfoKey : public PfkBakDbKeys {
    PfkBackupInfoKey(void) :
        PfkBakDbKeys(PREFIX_BACKUP_INFO),
        backup_number(this) {}
    BST_UINT32_t   backup_number;   /**< from PfkBackupDbInfoData.backups */
};

/** information about one generation in this backup. */
struct PfkBakGenInfo : public BST {
    PfkBakGenInfo(BST *parent) :
        BST(parent),
        date_time(this), generation_number(this) { }
    BST_STRING    date_time;   /**< formatted string of the date */
    BST_UINT32_t  generation_number;
};

/** data class for information about each backup.
 * the data class lists the root dir, list of generations, comment,
 * next generation number, etc.
 */
struct PfkBackupInfoData : public FileBlockBST {
    PfkBackupInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        root_dir(this), name(this), comment(this), file_count(this),
        next_generation_number(this), generations(this) {}
    ~PfkBackupInfoData(void) { bst_free(); }
    BST_STRING    root_dir;   /**< root directory for this backup */
    BST_STRING    name;       /**< short text name for the backup */
    BST_STRING    comment;    /**< comment string */
    BST_UINT32_t  file_count;  /**< number of files in this backup */
    BST_UINT32_t  next_generation_number;
    BST_ARRAY <PfkBakGenInfo> generations;  /**< list of generations */
};

/** \class PfkBackupInfo
 * This is a container for PfkBackupInfoKey and PfkBackupInfoData,
 * derived from DB_ITEM. */
DB_ITEM_CLASS(PfkBackupInfo);

// PfkBackupFileInfo - One of these for each file in a backup,
//   file_number walks from 0 to PfkBackupInfoData::file_count-1.
//   this is used to opimize tree-walking, for comparing size and mtime
//   to determine if a file has changed since the last backup run.

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
        file_path(this), mode(this), size(this),
        mtime(this), generations(this) { }
    ~PfkBackupFileInfoData(void) { bst_free(); }
    BST_STRING     file_path;
    BST_UINT16_t   mode;
    BST_UINT64_t   size;
    BST_UINT32_t   mtime;
    BST_ARRAY  <BST_UINT32_t>  generations;
};

DB_ITEM_CLASS(PfkBackupFileInfo);

// PfkBackupFilePieceInfo - one of these for each "piece" of a file
//   in a backup, describing all of the different versions of this
//   piece and their md5 hashes.

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
    BST_MD5         md5hash;
};

struct PfkBackupFilePieceInfoData : public FileBlockBST {
    PfkBackupFilePieceInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        versions(this) { }
    ~PfkBackupFilePieceInfoData(void) { bst_free(); }
    BST_ARRAY <PfkBackupVersion> versions;
};

DB_ITEM_CLASS(PfkBackupFilePieceInfo);

// PfkBackupFilePieceData - one of these for each version of each piece
//    of each file in a backup. contains a reference count and a pointer 
//    to the data.  the data is stored directly as an AUID in the FileBlock,
//    to make access more efficient when a file has not changed from one
//    generation to the next (simply increase the refcount without actually
//    retrieving and storing the data).

struct PfkBackupFilePieceDataKey : public PfkBakDbKeys {
    PfkBackupFilePieceDataKey(void) : 
        PfkBakDbKeys(PREFIX_FILE_PIECE_DATA),
        backup_number(this), file_number(this),
        piece_number(this), md5hash(this) { }
    BST_UINT32_t    backup_number;
    BST_UINT32_t    file_number;
    BST_UINT32_t    piece_number;
    BST_MD5         md5hash;
};

struct PfkBackupFilePieceDataData : public FileBlockBST {
    PfkBackupFilePieceDataData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        refcount(this), csize(this), usize(this), data_fbn(this) { }
    ~PfkBackupFilePieceDataData(void) { bst_free(); }
    BST_UINT32_t   refcount;
    BST_UINT16_t   csize;
    BST_UINT16_t   usize;
    BST_UINT32_t   data_fbn;

    static const int PIECE_SIZE = 60000;
};

DB_ITEM_CLASS(PfkBackupFilePieceData);

#endif /* __DATABASE_ELEMENTS_H__ */
