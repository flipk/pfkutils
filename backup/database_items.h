/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <time.h>
#include <iostream>

struct bakKey : public BST_UNION {
    enum { DBINFO, VERSIONINFO, VERSIONINDEX, FILEINFO, BLOBHASH, MAX };
    bakKey(void) : BST_UNION(NULL,MAX), dbinfo(this), versioninfo(this),
                   versionindex(this), fileinfo(this), blobhash(this) { }
    struct dbinfo_key : public BST {
        dbinfo_key(BST *parent) : BST(parent), keystring(this) { }
        BST_STRING keystring;
        void init(void) { keystring.string = "BKDBINFOKEY"; }
    } dbinfo;
    struct versioninfo_key : public BST {
        versioninfo_key(BST *parent) : BST(parent),
                                       version(this) { }
        BST_UINT32_t version;
    } versioninfo;
    struct versionindex_key : public BST {
        versionindex_key(BST *parent) : BST(parent),
                                        version(this), group(this) { }
        BST_UINT32_t version;
        BST_UINT32_t group;
    } versionindex;
    struct fileinfo_key : public BST {
        fileinfo_key(BST *parent) : BST(parent),
                                    version(this), filename(this) { }
        BST_UINT32_t version;
        BST_STRING filename;
    } fileinfo;
    struct blobhash_key : public BST {
        blobhash_key(BST *parent) : BST(parent),
                                    hash(this), filesize(this) { }
        BST_STRING  hash;
        BST_UINT64_t filesize;
    } blobhash;
};

struct bakData : public BST_UNION {
    enum { DBINFO, VERSIONINFO, VERSIONINDEX, FILEINFO, BLOBHASH, MAX };
    bakData(void) : BST_UNION(NULL,MAX), dbinfo(this), versioninfo(this),
                    versionindex(this), fileinfo(this), blobhash(this) { }
    struct dbinfo_data : public BST {
        dbinfo_data(BST *parent)
            : BST(parent), sourcedir(this), nextver(this), versions(this) { }
        BST_STRING sourcedir;
        BST_UINT32_t  nextver;
        BST_ARRAY<BST_UINT32_t>  versions;
    } dbinfo;
    struct versioninfo_data : public BST {
        versioninfo_data(BST *parent) : BST(parent), time(this),
                                        filecount(this), total_bytes(this) { }
        BST_TIMEVAL  time;
        BST_UINT32_t filecount;
        BST_UINT64_t total_bytes;
    } versioninfo;
    struct versionindex_data : public BST {
        versionindex_data(BST *parent) : BST(parent), filenames(this) { }
        static const int MAX_FILENAMES = 100;
        BST_ARRAY<BST_STRING>  filenames;
    } versionindex;
    struct fileinfo_data : public BST {
        fileinfo_data(BST *parent) : BST(parent), hash(this), time(this),
                                     filesize(this) { }
        BST_STRING hash;
        BST_TIMEVAL time;
        BST_UINT64_t filesize;
    } fileinfo;
    struct blobhash_data : public BST {
        blobhash_data(BST *parent) : BST(parent), refcount(this),
                                     first_auid(this) { }
        BST_UINT32_t refcount;
        BST_FB_AUID_t first_auid;
    } blobhash;
};

struct bakFileContents : public BST {
    bakFileContents(void) : BST(NULL), data(this), next_auid(this) { }
    BST_STRING data;
    BST_FB_AUID_t next_auid;
};

class bakDatum {
    Btree * bt;
    FileBlockInterface * fbi;
    FB_AUID_T data_id;
    bool dirty;
public:
    bakKey key;
    bakData data;

    bakDatum(Btree * _bt) : bt(_bt), data_id(0), dirty(false) {
        fbi = bt->get_fbi();
    }

    ~bakDatum(void) { if (dirty) put(); }

    void reinit(void) { data_id = 0; dirty = false; }
    void mark_dirty(void) { dirty = true; }

    // fill out key item before calling this; if it returns
    // true, the data was found and is populated.
    bool get(void) {
        if (bt->get(&key,&data_id) == false)
        {
            data_id = 0;
            return false;
        }
        return get_data(data_id);
    }

    bool get_data(FB_AUID_T id) {
        FileBlock * datafb = fbi->get(id,false);
        if (datafb == false)
        {
            std::cerr << "fbi->get(data_id) failed!\n";
            return false;
        }
        bool ret = true;
        if (data.bst_decode(datafb->get_ptr(),
                            datafb->get_size()) == false)
        {
            std::cerr << "data.bst_decode failed!\n";
            ret = false;
        }
        else
        {
            if (data.which.v != key.which.v)
            {
                std::cerr << "data.which.v != key.which.v!\n";
                ret = false;
            }
        }
        fbi->release(datafb,false);
        dirty = false;
        return ret;
    }

    // fill out key and data items before calling this.
    void put(void) {
        int datalen = data.bst_calc_size();
        if (data_id != 0)
            fbi->realloc(data_id, datalen);
        else
            data_id = fbi->alloc(datalen);
        FileBlock * fb = fbi->get(data_id,true);
        if (fb)
        {
            int len = fb->get_size();
            data.bst_encode(fb->get_ptr(),&len);
            fbi->release(fb,true);
        }
        dirty = false;
        bool replaced = false;
        FB_AUID_T old_data_id = 0;
        bt->put(&key, data_id, /*replace*/true,
                &replaced, &old_data_id);
        if (replaced &&
            old_data_id != 0 &&
            old_data_id != data_id)
        {
            fbi->free(old_data_id);
        }
    }

    // fill out key before calling this.
    void del(void) {
        FB_AUID_T old_data_id = 0;
        if (bt->del(&key, &old_data_id) == false) {
            std::cerr << "error deleting? what now?\n";
        } else {
            fbi->free(old_data_id);
        }
        dirty = false;
        data_id = 0;
    }
};

static inline std::string
format_hash(const std::string &hashbin)
{
    unsigned char * hash = (unsigned char *) hashbin.c_str();
    std::string str;
    str.resize(hashbin.size() * 2);
    char * buf = (char*) str.c_str();
    for (int ind = 0; ind < hashbin.size(); ind++)
        sprintf(buf + (ind*2), "%02x", hash[ind]);
    return str;
}

static inline std::string
format_time(const myTimeval &tv)
{
    char str[80];
    struct tm tmtime;
    localtime_r(&tv.tv_sec, &tmtime);
    strftime(str,sizeof(str), "%Y-%m-%d %H:%M:%S", &tmtime);
    sprintf(str+strlen(str), ".%06d", tv.tv_usec);
    return str;
}