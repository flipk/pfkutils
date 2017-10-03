
#include "PageCache.H"
#include "PageCache_internal.H"

#include <string.h>

PageCache :: PageCache( PageIO * _io, int _max_pages )
{
    io = _io;
    max_pages = _max_pages;
    pgs = new PageCachePageList;
}

PageCache :: ~PageCache(void)
{
    PCPInt * p;
    flush();
    while ((p = pgs->get_head()) != NULL)
    {
        if (p->is_locked())
        {
            fprintf(stderr, "error page is locked at delete time!\n");
            exit(1);
        }
        pgs->remove(p);
        delete p;
    }
    delete pgs;
}

PageCachePage *
PageCache :: get(int page_number, bool for_write)
{
    PCPInt * ret;
    ret = pgs->find( page_number );
    if (ret != NULL)
    {
        pgs->ref(ret);
        return ret;
    }
    ret = new PCPInt( page_number );
    if (for_write)
    {
        memset(ret->ptr, 0, PAGE_SIZE);
        ret->dirty = true;
    }
    else
        if (!io->get_page(ret))
        {
            fprintf(stderr, "error getting page %d\n", page_number);
            exit( 1 );
        }
    pgs->add(ret,true);
    return ret;
}

void
PageCache :: release( PageCachePage * _p, bool dirty )
{
    PCPInt * p = (PCPInt *)_p;
    if (dirty)
        p->dirty = true;
    p->deref();
    while (pgs->get_lru_cnt() > max_pages)
    {
        p = pgs->get_oldest();
        pgs->remove(p);
        if (p->dirty)
            if (!io->put_page(p))
            {
                fprintf(stderr, "error putting page %d\n",
                        p->get_page_number());
                exit( 1 );
            }
        delete p;
    }
}

static int
page_compare( const void * _a, const void * _b )
{
    PCPInt * a = *(PCPInt **)_a;
    PCPInt * b = *(PCPInt **)_b;
    if (a->get_page_number() > b->get_page_number())
        return 1;
    if (a->get_page_number() < b->get_page_number())
        return -1;
    return 0;
}

void
PageCache :: flush(void)
{
    PCPInt * p;
    int i, count;

    count = pgs->get_cnt();
    PCPInt * pages[count];
    i = 0;
    for (p = pgs->get_head(); p; p = pgs->get_next(p))
        if (p->dirty)
            pages[i++] = p;
    count = i;

    qsort( pages, count, sizeof(PCPInt*),
           (int(*)(const void *, const void *))page_compare );

    for (i = 0; i < count; i++)
    {
        p = pages[i];
        if (!io->put_page(p))
        {
            fprintf(stderr, "error putting page %d\n",
                    p->get_page_number());
            exit( 1 );
        }
    }
}
