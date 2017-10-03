
/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

#include "bst.H"
#include "Btree.H"
#include "FileBlock_iface.H"

struct DatabaseKeys : public BST {
    enum Prefixes {
        PREFIX_BACKUP_INFO_LIST,
        PREFIX_BACKUP_INFO,
        PREFIX_DIR_INFO,
        PREFIX_FILE_INFO,
        PREFIX_PIECE_INFO
    };
    BST_UINT8_t    prefix;
};

struct BackupInfoListKey : public DatabaseKeys {
    static const unsigned char myprefix = PREFIX_BACKUP_INFO_LIST;
    // no additional data required for this key
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &prefix, NULL };
        return bst_do_fields( str, fields );
    }
};

struct BackupInfoListData : public FileBlockBST {
    BackupInfoListData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_VAR_ARRAY <BST_UINT32_t>   backup_ids;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &backup_ids, NULL };
        return bst_do_fields( str, fields );
    }
};

struct BackupInfoKey : public DatabaseKeys {
    static const unsigned char myprefix = PREFIX_BACKUP_INFO;
    BST_UINT32_t   backup_id;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &prefix, &backup_id, NULL };
        return bst_do_fields( str, fields );
    }
};

struct GenerationInfo : public BST {
    BST_UINT64_t   num_bytes;
    BST_UINT32_t   num_files;
    BST_UINT32_t   root_dir_id;
    BST_STRING     date_time;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &num_bytes, &num_files,
                           &root_dir_id, &date_time, NULL };
        return bst_do_fields( str, fields );
    }
};

struct BackupInfoData : public FileBlockBST {
    BackupInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_STRING     backup_name;
    BST_VAR_ARRAY <GenerationInfo>  generations;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &backup_name, &generations, NULL };
        return bst_do_fields( str, fields );
    }
};

struct DirectoryInfoKey : public DatabaseKeys {
    static const unsigned char myprefix = PREFIX_DIR_INFO;
    BST_UINT32_t  dir_id;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &prefix, &dir_id, NULL };
        return bst_do_fields( str, fields );
    }
};

struct DirectoryFileUnion : public BST_UNION {
    enum { DIRECTORY, FILE, LINK, MAX };
    DirectoryFileUnion(void) : BST_UNION(MAX) { }
    ~DirectoryFileUnion(void) { bst_free(); }
    BST_UINT32_t   dir_id;
    BST_UINT32_t   file_id;
    BST_STRING     link_target;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST * fields[] = { &dir_id, &file_id, &link_target };
        return bst_do_union(str, fields);
    }
};

struct DirectoryEntry : public BST {
    BST_STRING          name;
    DirectoryFileUnion  info; 
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &name, &info, NULL };
        return bst_do_fields( str, fields );
    }
};

struct DirectoryInfoData : public FileBlockBST {
    DirectoryInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_VAR_ARRAY <DirectoryEntry>  entries;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &entries, NULL };
        return bst_do_fields( str, fields );
    }
};

struct FileInfoKey : public DatabaseKeys {
    static const unsigned char myprefix = PREFIX_FILE_INFO;
    BST_UINT32_t  file_id;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &prefix, &file_id, NULL };
        return bst_do_fields( str, fields );
    }
};

typedef BST_FIXED_BINARY <16> BST_MD5_HASH;

/* when piece list doesn't fit in FileInfoData, the list
 * extends into a linked list of these. */
struct FilePieceListData : public FileBlockBST {
    FilePieceListData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_VAR_ARRAY  <BST_MD5_HASH> pieces;
    BST_UINT32_t  next_pieces_fbn;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &pieces, &next_pieces_fbn, NULL };
        return bst_do_fields( str, fields );
    }
};

struct FileInfoData : public FileBlockBST {
    FileInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_UINT64_t  size;
    BST_UINT32_t  atime;
    BST_UINT32_t  num_pieces;
//    static const int max_pieces_per_fbn = 2000;
    static const int max_pieces_per_fbn = 20; // for testing the functionality
    BST_VAR_ARRAY  <BST_MD5_HASH> pieces;
    BST_UINT32_t  next_pieces_fbn;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &size, &atime, &num_pieces,
                           &pieces, &next_pieces_fbn, NULL };
        return bst_do_fields( str, fields );
    }
};

struct PieceInfoKey : public DatabaseKeys {
    static const unsigned char myprefix = PREFIX_PIECE_INFO;
    BST_MD5_HASH   hash;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &prefix, &hash, NULL };
        return bst_do_fields( str, fields );
    }
};

struct PieceInfoData : public FileBlockBST {
    PieceInfoData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_UINT32_t   reference_count;
    BST_UINT16_t   comp_size;  // compressed size
    BST_UINT16_t   uncomp_size;  // uncompressed size
    BST_UINT32_t   data_fbn;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &reference_count, &comp_size,
                           &uncomp_size, &data_fbn, NULL };
        return bst_do_fields( str, fields );
    }
};

struct BinaryData : public FileBlockBST {
    BinaryData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    BST_VAR_BINARY   data;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &data, NULL };
        return bst_do_fields( str, fields );
    }
};
