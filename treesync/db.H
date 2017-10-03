
#ifndef __DB_H__
#define __DB_H__

#include "bst.H"
#include "Btree.H"
#include "BtreeDbClasses.H"

struct TreeSyncDbKeys : public BST {
    enum Prefix {
        PREFIX_DB_INFO,
        PREFIX_FILE_SEQ
    };
    TreeSyncDbKeys( Prefix _p ) :
        BST(NULL), prefix(this) { prefix.v = _p; }
    BST_UINT8_t    prefix;   /**< MUST be included in BSG_FIELD_LIST of
                                any derived type. */
};

// singleton -> num_files
// index -> file_path, mode, size, mtime, md5hash
//    (where index is 0-N)

#define INFO_KEY  "TreeSyncDbInfo"

struct TreeSyncDbInfoKey : public TreeSyncDbKeys {
    TreeSyncDbInfoKey(void) :
        TreeSyncDbKeys(PREFIX_DB_INFO), info_key(this) { }
    BST_STRING   info_key;
};

#define TOOL_VERSION 2

struct TreeSyncDbInfoData : public FileBlockBST {
    TreeSyncDbInfoData(FileBlockInterface * fbi) :
        FileBlockBST(NULL,fbi), num_files(this), tool_version(this) { }
    ~TreeSyncDbInfoData(void) { bst_free(); }
    BST_UINT32_t   num_files;
    BST_UINT16_t   tool_version;
};

DB_ITEM_CLASS(TreeSyncDbInfo);

struct TreeSyncFileInfoKey : public TreeSyncDbKeys {
    TreeSyncFileInfoKey(void) :
        TreeSyncDbKeys(PREFIX_FILE_SEQ), index(this) { }
    BST_UINT32_t   index;
};

struct TreeSyncFileInfoData : public FileBlockBST {
    TreeSyncFileInfoData(FileBlockInterface *fbi) :
        FileBlockBST(NULL,fbi),
        state(this), file_path(this), size(this), mtime(this), md5hash(this) { }
    ~TreeSyncFileInfoData(void) { bst_free(); }
    enum { STATE_EXISTS=1, STATE_DELETED=2 };
    BST_UINT8_t    state;
    BST_STRING     file_path;
    BST_UINT64_t   size;
    BST_UINT32_t   mtime;
    BST_FIXED_BINARY <MD5_DIGEST_SIZE> md5hash;
};

DB_ITEM_CLASS(TreeSyncFileInfo);

#endif /* __DB_H__ */
