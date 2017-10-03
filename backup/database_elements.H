
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

#define TOOL_VERSION 1

/** base type for any btree key. every btree key will have a prefix
 * byte which is unique to the type.  This struct's constructor ensures
 * that the prefix byte is populated.
 * \note Every type which derives from this MUST include the "prefix"
 *    field in the BST_FIELD_LIST.
 */
struct DatabaseKeys : public BST {
    enum Prefix {
        PREFIX_BACKUP_INFO_LIST,   /**< used by BackupInfoListKey */
        PREFIX_BACKUP_INFO,        /**< used by BackupInfoKey     */
        PREFIX_GENERATION_INFO,
        PREFIX_DIR_INFO,           /**< used by DirectoryInfoKey  */
        PREFIX_FILE_INFO           /**< used by FileInfoKey       */
    };
    DatabaseKeys( Prefix _p ) { prefix.v = _p; }
    BST_UINT8_t    prefix;   /**< MUST be included in BSG_FIELD_LIST of
                                any derived type. */
};

/** this is the very first data structure in the heirarchy of data
 * structures found in a backup.  this maps to BackupInfoListData
 * which will list all the individual backups found in this file. */
struct BackupFileInfoKey : public DatabaseKeys {
    BackupFileInfoKey(void) : DatabaseKeys(PREFIX_BACKUP_INFO_LIST) { }
    // no additional data required for this key
    BST_FIELD_LIST( &prefix );
};

/** companion to BackupInfoListKey, this lists all the backup identifiers
 * found in the file.  look each backup ID individually using
 * BackupInfoKey. */
struct BackupFileInfoData : public FileBlockBST {
    BackupFileInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~BackupFileInfoData(void) { bst_free(); }
    BST_UINT32_t   tool_version;  /**< version# of pfkbak that created it */
    BST_VAR_ARRAY <BST_UINT32_t> backup_numbers;
    BST_FIELD_LIST( &tool_version, &backup_numbers );
};

DB_ITEM_CLASS(BackupFileInfo);

/** linked from BackupInfoListData, this lets you locate
 * a BackupInfoData which lists all the generations found in a backup. */
struct BackupInfoKey : public DatabaseKeys {
    BackupInfoKey(void) : DatabaseKeys(PREFIX_BACKUP_INFO) { }
    BST_UINT32_t   backup_number;
    BST_FIELD_LIST( &prefix, &backup_number );
};

/** companion to BackupInfoKey, this lists info about a backup, such
 * as its name, pointer to root directory, and contains a list of all
 * active generations. */
struct BackupInfoData : public FileBlockBST {
    BackupInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~BackupInfoData(void) { bst_free(); }
    BST_STRING     backup_name;            /**< text name of this backup. */
    BST_STRING     comment;
    BST_VAR_ARRAY <BST_UINT32_t>   generation_numbers;
    BST_FIELD_LIST( &backup_name, &comment, &generation_numbers );
};

DB_ITEM_CLASS(BackupInfo);

struct GenerationInfoKey : public DatabaseKeys {
    GenerationInfoKey(void) : DatabaseKeys(PREFIX_GENERATION_INFO) { }
    BST_UINT32_t  backup_number;
    BST_UINT32_t  generation_number;
    BST_FIELD_LIST( &prefix, &backup_number, &generation_number );
};

struct GenerationInfoData : public BST {
    BST_STRING     date_time;       /**< when was this generation taken */
    BST_UINT64_t   num_bytes;       /**< number of bytes in generation  */
    BST_UINT32_t   num_files;       /**< number of files in generation  */
//xxx
    BST_FIELD_LIST( &date_time, &num_bytes, &num_files /*xxx*/ );
};

/** linked from GenerationInfo as well as from other DirectoryEntry. 
 * associated with DirectoryInfoData. */
struct DirectoryInfoKey : public DatabaseKeys {
    DirectoryInfoKey(void) : DatabaseKeys(PREFIX_DIR_INFO) { }
    BST_UINT32_t  dir_id;  /**< from GenerationInfo or DirectoryEntry */
    BST_FIELD_LIST( &prefix, &dir_id );
};

/** entry in a directory can be either a dir, file, or symlink. handle
 * that as a union. */
struct DirectoryFileUnion : public BST_UNION {
    enum { DIRECTORY, FILE, LINK, MAX };
    DirectoryFileUnion(void) : BST_UNION(MAX) { }
    BST_UINT32_t   dir_id;        /**< if directory, locate DirectoryInfoKey */
    BST_UINT32_t   file_id;       /**< if file, locate a FileInfoKey         */
    BST_STRING     link_target;   /**< if link, target is right here         */
    BST_UNION_LIST( &dir_id, &file_id, &link_target );
};

/** one directory entry */
struct DirectoryEntry : public BST {
    BST_STRING          name;    /**< the name of the file, dir, or link */
    DirectoryFileUnion  info;    /**< more data about the item here      */
    BST_FIELD_LIST( &name, &info );
};

/** a whole directory.  note that these are unique in a generation,
 * that is to say a given dir is found only in one generation.  the next
 * generation creates a whole new tree of these. */
struct DirectoryInfoData : public FileBlockBST {
    DirectoryInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~DirectoryInfoData(void) { bst_free(); }
    BST_VAR_ARRAY <DirectoryEntry>  entries;  /**< list of entries in dir */
    BST_FIELD_LIST( &entries );
};

/** information about a file, found in FileInfoData.
 * note that these are shared among trees.
 * you get here from DirectoryInfoData entries.  While every generation 
 * has a complete copy of the tree, FileInfoKey and FileInfoData are shared
 * among all the generations that have it.  that's also why there's a refcount
 * in the file info. */
struct FileInfoKey : public DatabaseKeys {
    FileInfoKey(void) : DatabaseKeys(PREFIX_FILE_INFO) { }
    BST_UINT32_t  file_id;  /**< id number of this file */
    BST_FIELD_LIST( &prefix, &file_id );
};

/** a data type for storing an MD5 hash. */
typedef BST_FIXED_BINARY <16> BST_MD5_HASH;

/** everything you need to know about one single 32k piece of data,
 * such as which generations it belongs to, etc.  found in FileInfoData. */
struct PieceVersions : public BST {
    BST_VAR_ARRAY <BST_UINT32_t>  generations;  /**< list all gen#s */
    BST_MD5_HASH   md5;       /**< hash of this piece */
    BST_UINT16_t   csize;     /**< compressed size of this piece */
    BST_UINT16_t   usize;     /**< uncompressed size of this piece */
    BST_FB_AUID_t  data_fbn;  /**< fbn of the BinaryData */
    BST_FIELD_LIST(&generations, &md5, &csize, &usize, &data_fbn);
};

/** info about a bunch of 32k pieces. find a linked list of these by
 * following the next pointer to describe a whole file.
 * found in FileInfoData. */
struct FilePieces : public BST {
    BST_VAR_ARRAY <PieceVersions> pieces;    /**< a group of pieces */
    BST_FB_AUID_t  next_pieces_fbn;    /**< pointer to FileInfoExtended */
    BST_FIELD_LIST(&pieces, &next_pieces_fbn);
};

/** data companion to FileInfoKey.  everything you ever wanted to know
 * about a file except its name (which is in the directory). */
struct FileInfoData : public FileBlockBST {
    FileInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~FileInfoData(void) { bst_free(); }
    BST_UINT64_t  size;          /**< size of this file in bytes */
    BST_UINT32_t  atime;         /**< last access time of this file (time_t) */
    BST_UINT32_t  num_pieces;    /**< number of 32k piece descriptors */
    BST_UINT16_t  refcount;      /**< how many directories (generations)
                                    reference this file */
    BST_UINT16_t  file_mode;     /**< as in chmod kind of mode */
    BST_FIELD_LIST( &size, &atime, &num_pieces, &refcount, &file_mode );
};

/** if there wasn't enough room in FileInfoData for all the pieces of a 
 * large file, follow next_pieces_fbn to find more of these.  */
struct FileInfoExtended : public FileBlockBST {
    FileInfoExtended(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~FileInfoExtended(void) { bst_free(); }
    FilePieces    next_pieces;   /**< group of piece descriptors */
    BST_FIELD_LIST( &next_pieces );
};

/** finally at the end of all that: the libz-compressed 32kb data piece
 * of a file. */
struct BinaryData : public FileBlockBST {
    BinaryData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~BinaryData(void) { bst_free(); }
    BST_VAR_BINARY   data;
    BST_FIELD_LIST( &data );
};

DB_ITEM_CLASS(DirectoryInfo);
DB_ITEM_CLASS(FileInfo);

#endif /* __DATABASE_ELEMENTS_H__ */
