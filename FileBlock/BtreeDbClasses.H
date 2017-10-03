
#ifndef __BTREE_DB_CLASSES_H__
#define __BTREE_DB_CLASSES_H__

class DB_ITEM {
    Btree * bt;
protected:
    DB_ITEM(Btree * _bt) { bt = _bt; }
    bool _put(BST * key, FileBlockBST * data, bool replace) {
        FB_AUID_T data_fbn;
        if (data->putnew( &data_fbn ) == false)
        {
            printf("putnew failure\n");
            return false;
        }
        bool replaced;
        FB_AUID_T old_data_id;
        if (bt->put(key, data_fbn, replace,
                    &replaced, &old_data_id) == false)
        {
            printf("bt put failure\n");
            return false;
        }
        if (replace && replaced)
        {
            bt->get_fbi()->free(old_data_id);
        }
        return true;
    }
    bool _get(BST * key, FileBlockBST * data) {
        FB_AUID_T data_fbn;
        if (bt->get(key, &data_fbn) == false)
            return false;
        if (data->get(data_fbn) == false)
            return false;
        return true;
    }
    bool _del(BST * key, FileBlockBST * data) {
        FB_AUID_T  data_fbn;
        if (bt->del(key, &data_fbn)==false)
            return false;
        bt->get_fbi()->free(data_fbn);
        return true;
    }
};


#define DB_ITEM_CLASS(datum)                                            \
    class datum : public DB_ITEM {                                      \
    public:                                                             \
        datum(Btree * _bt) : DB_ITEM(_bt), data(_bt->get_fbi()) { }     \
        datum##Key key;                                                 \
        datum##Data data;                                               \
        bool put(bool repl=false) { return _put(&key, &data, repl); }   \
        bool get(void)            { return _get(&key, &data); }         \
        bool del(void)            { return _del(&key, &data); }         \
    }

/*

To use DB_ITEM_CLASS and DB_ITEM, do the following:

- define a DatabaseKeys base class containing only a prefix,
  an enum, and a constructor which takes one of the enum values.

- define a Key struct derived from this keys base class, which
  calls the base class construtor with a unique prefix enum value
  to identify the structure. add elements for the key, and
  a BST_FIELD_LIST which includes the prefix from the base class.

- define a Data struct derived from FileBlockBST; the base class
  constructor of course requires the FileBlockInterface pointer.
  add a BST_FIELD_LIST to this one for all the fields as well.
  and remember: types derived from FileBlockBST must call free
  in their destructors!

- invoke DB_ITEM_CLASS(myClass) macro; this automatically creates
  a myClass type definition which is derived from DB_ITEM and 
  which contains myClassKey and myClassData.

- when instantiating myClass you must specify the Btree* pointer.

- you can now populate the myClass.key member and call myClass.get,
  or populate both .key and .data and call myClass.put.

 */

#endif /* __BTREE_DB_CLASSES_H__ */
