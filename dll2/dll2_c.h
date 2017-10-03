
typedef struct {
    void * next;
    void * prev;
    struct DLL2_LIST * onlist;
} DLL2_LINKS;

typedef struct DLL2_LIST {
    void * head;
    void * tail;
    int count;
    int index;
} DLL2_LIST;

typedef struct {
    int count;
    int array_size;
    DLL2_LIST array[0];
}  DLL2_HASH;

/*
  some compilers (like gcc version 3) don't allow you to 
  use the traditional "offsetof" operator:
  -  #define offsetof(type,field) ((unsigned int)&(((type*)0)->field))
  an alternative that slips by gcc's rules would be:
  -  #define offsetof(type,field) ((unsigned int)&(((type*)1)->field)-1)
  but we're skirting the issue by actually using (&(ptr->field) - ptr).
*/

#define DLL2_LINKS_OFFSET(item) \
    (((unsigned int)((item)->links)) - ((unsigned int)(item)))

#define DLL2_LIST_INIT(list,listindex) \
    do { \
        (list)->head = (list)->tail = 0; \
        (list)->count = 0; \
        (list)->index = listindex; \
    } while ( 0 )
#define DLL2_LINKS_INIT(item) \
    memset((item)->links,0,sizeof((item)->links))

#define DLL2_ONLIST(listindex,item) \
    ((item)->links[listindex].onlist != 0)
#define DLL2_ONTHISLIST(list,item) \
   ((item)->links[(list)->index].onlist == list)

#define DLL2_ADD(list,item) \
    dll2_list_add((list),DLL2_LINKS_OFFSET(item),(item),__FILE__,__LINE__)
#define DLL2_DEL(list,item) \
    dll2_list_del((list),DLL2_LINKS_OFFSET(item),(item),__FILE__,__LINE__)
#define DLL2_ADD_AFTER(list,existing,item)   \
    dll2_list_add_after((list),DLL2_LINKS_OFFSET(item), \
                        (existing),(item),__FILE__,__LINE__)
#define DLL2_ADD_BEFORE(list,existing,item)  \
    dll2_list_add_before((list),DLL2_LINKS_OFFSET(item), \
                         (existing),(item),__FILE__,__LINE__)

#define DLL2_SIZE(list)              ((list)->count)

#define DLL2_HEAD(list)              ((list)->head)
#define DLL2_TAIL(list)              ((list)->tail)

#define DLL2_NEXT(list,item) \
    (((item)->links[(list)->index].onlist==list)? \
     ((item)->links[(list)->index].next) : \
      dll2_error( __FILE__, __LINE__, "dll2_next wrong list" ))
#define DLL2_PREV(list,item) \
    (((item)->links[(list)->index].onlist==list)? \
     ((item)->links[(list)->index].prev) : \
      dll2_error( __FILE__, __LINE__, "dll2_prev wrong list" ))

extern void dll2_list_add( DLL2_LIST * list, int links_offset,
                           void * item, char * file, int line );
extern void dll2_list_del( DLL2_LIST * list, int links_offset,
                           void * item, char * file, int line );
extern void dll2_list_add_after( DLL2_LIST * list, int links_offset,
                                 void * existing, void * item,
                                 char * file, int line );
extern void dll2_list_add_before( DLL2_LIST * list, int links_offset,
                                  void * existing, void * item,
                                  char * file, int line );
/* this function only has a void * return type to eliminate certain
   warnings in the above macros which use the ternary ?: operator */
extern void * dll2_error( char * file, int line, char * format, ... );
