
#ifndef __DB_H__
#define __DB_H__

#include "bst.H"
#include "Btree.H"
#include "BtreeDbClasses.H"

struct DatabaseKeys : public BST {
    enum Prefix {
        PREFIX_DB_INFO,
        PREFIX_FILE_SEQ
    };
    DatabaseKeys( Prefix _p ) { prefix.v = _p; }
    BST_UINT8_t    prefix;   /**< MUST be included in BSG_FIELD_LIST of
                                any derived type. */
};

// singleton -> num_files
// index -> file_path, mode, size, mtime, md5hash
//    (where index is 0-N)

struct DbInfoKey : public DatabaseKeys {
    DbInfoKey(void) : DatabaseKeys(PREFIX_DB_INFO) { }
    // nothing
    BST_FIELD_LIST( &prefix );
};

struct DbInfoData : public FileBlockBST {
    DbInfoData(FileBlockInterface * fbi) : FileBlockBST(fbi) { }
    ~DbInfoData(void) { bst_free(); }
    BST_UINT32_t   num_files;
    BST_FIELD_LIST( &num_files );
};

DB_ITEM_CLASS(DbInfo);

struct FileSeqKey : public DatabaseKeys {
    FileSeqKey(void) : DatabaseKeys(PREFIX_FILE_SEQ) { }
    BST_UINT32_t   index;
    BST_FIELD_LIST( &prefix, &index );
};

struct FileSeqData : public FileBlockBST {
    FileSeqData(FileBlockInterface *fbi) : FileBlockBST(fbi) { }
    ~FileSeqData(void) { bst_free(); }
    BST_STRING   file_path;
    BST_UINT64_t   size;
    BST_UINT32_t   mtime;
    BST_FIXED_ARRAY  <BST_UINT8_t,16>  md5hash;
    BST_FIELD_LIST( &file_path, &size, &mtime, &md5hash );
};

DB_ITEM_CLASS(FileSeq);

#endif /* __DB_H__ */
