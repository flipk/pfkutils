
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
    DatabaseKeys( Prefix _p ) :
        BST(NULL), prefix(this) { prefix.v = _p; }
    BST_UINT8_t    prefix;   /**< MUST be included in BSG_FIELD_LIST of
                                any derived type. */
};

// singleton -> num_files
// index -> file_path, mode, size, mtime, md5hash
//    (where index is 0-N)

#define INFO_KEY  "TreeSyncDbInfo"

struct DbInfoKey : public DatabaseKeys {
    DbInfoKey(void) :
        DatabaseKeys(PREFIX_DB_INFO), info_key(this) { }
    BST_STRING   info_key;
};

#define TOOL_VERSION 2

struct DbInfoData : public FileBlockBST {
    DbInfoData(FileBlockInterface * fbi) :
        FileBlockBST(NULL,fbi), num_files(this), tool_version(this) { }
    ~DbInfoData(void) { bst_free(); }
    BST_UINT32_t   num_files;
    BST_UINT16_t   tool_version;
};

DB_ITEM_CLASS(DbInfo);

struct FileSeqKey : public DatabaseKeys {
    FileSeqKey(void) :
        DatabaseKeys(PREFIX_FILE_SEQ), index(this) { }
    BST_UINT32_t   index;
};

struct FileSeqData : public FileBlockBST {
    FileSeqData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        state(this), file_path(this), size(this), mtime(this), md5hash(this) { }
    ~FileSeqData(void) { bst_free(); }
    enum { STATE_EXISTS=1, STATE_DELETED=2 };
    BST_UINT8_t    state;
    BST_STRING     file_path;
    BST_UINT64_t   size;
    BST_UINT32_t   mtime;
    BST_FIXED_BINARY <MD5_DIGEST_SIZE> md5hash;
};

DB_ITEM_CLASS(FileSeq);

#endif /* __DB_H__ */
