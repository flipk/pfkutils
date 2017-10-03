
/** \file PageCache.C
 * \brief Implements PageCache and PageIOFileDescriptor.
 * \author Phillip F Knaack */

#include "PageCache.H"
#include "PageCache_internal.H"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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
        if (for_write)
        {
            memset(ret->ptr, 0, PAGE_SIZE);
            ret->dirty = true;
        }
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
    pgs->deref(p);
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

/** compare page numbers of two PCPInt objects, for qsort.
 * \param _a the first PCPInt to compare
 * \param _b the second PCPInt to compare
 * \return positive if _a's page number is greator than _b's page number,
 *         negative if _a's page number is less than _b's page number,
 *         or zero if they are equal.
 * \relates PageCache
 *
 * This function compares two PCPInt objects.  Its purpose is to be a
 * utility function to the standard C library function qsort, to assist
 * qsort in sorting an array of PCPInt objects.  */

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
        // the page is now synced with the file.
        p->dirty = false;
    }
}
