
template <class C>
class linked_list {
    C * head;
    C * tail;
    int len;
    ThreadSemaphore * sem;
    char * name;
public:
    linked_list( char * _name )
        {
            len = 0;
            head = NULL;
            tail = NULL;
            sem = NULL;
            name = _name;
        }
    ~linked_list( void )
        {
            if ( sem )
                th->sems->semdelete( sem );
        }
    void add( C * item )
        {
            if ( !sem )
                sem = th->sems->seminit( name, 1 );
            sem->take();
            item->next = NULL;
            item->prev = tail;
            if ( tail )
                tail->next = item;
            tail = item;
            if ( !head )
                head = tail;
            len++;
            sem->give();
        }
    void del( C * item )
        {
            sem->take();
            if ( item->prev )
                item->prev->next = item->next;
            else
                head = item->next;
            if ( item->next )
                item->next->prev = item->prev;
            else
                tail = item->prev;
            len--;
            sem->give();
        }
    C * iter_start( void )
        {
            if ( sem )
                sem->take();
            return head;
        }
    void iter_done( void )
        {
            if ( sem )
                sem->give();
        }
};
