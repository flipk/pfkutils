/*
 * memory pool management.
 */

#include "internal.h"

#define CALCBITNUMBER(bufnumber, bit, entry) \
	entry = bufnumber / 32;              \
	bit  = bufnumber % 32

__inline static int
validate_mp(mp, name)
	mpool *mp;
	char *name;
{
	if (mp == NULL)
	{
		fprintf(stderr, "%s: null mpool arg passed in\n", name);
		return 1;
	}

	if (mp->poolmagic != MPOOLMAGIC)
	{
		fprintf(stderr,
			"%s: pool magic number does not match\n", name);
		return 1;
	}

	return 0;
}

__inline static int
validate_dat(dat, name)
	void *dat;
	char *name;
{
	if (dat == NULL)
	{
		fprintf(stderr, "%s: null dat passed in\n", name);
		return 1;
	}

	return 0;
}

mpool *
pool_init(size, numelements)
	int size;
	int numelements;
{
	void *data;
	int est;

	est = pool_init_estimate(size, numelements);
	data = malloc(est);
	return _pool_init(size, numelements, data);
}

mpool *
_pool_init(size, numelements, addr)
	int size;
	int numelements;
	void *addr;
{
	mpool *ret;
	struct mpool_data *gen;
	void *bitmap_memory;
	void *ptr;

	if (size < 1)
	{
		fprintf(stderr, "pool_init: size too small\n");
		return NULL;
	}

	if (numelements < 1)
	{
		fprintf(stderr,
			"pool_init: numelements parameter not valid\n");
		return NULL;
	}

	/*
	 * pad size up to 8-byte boundary.
	 */

	size = ((size + 7) & ~7);

	ret = addr;

	if (ret == NULL)
	{
		fprintf(stderr, 
			"pool_init: couldn't allocate memory for mpool\n");
		return NULL;
	}

	ret->bitmap = ((void*)ret) + (sizeof(mpool) + size * numelements);

	if (ret->bitmap == NULL)
	{
		fprintf(stderr, 
			"pool_init: couldn't allocate memory for bitmap\n");
		free(ret);
		return NULL;
	}

	bzero(ret->bitmap, (numelements / 32 + 1) * 4);

	ret->size = size;
	ret->totalsize = size * numelements;
	ret->start = (void *)(ret + 1);
	ret->end = ret->start + ret->totalsize - size;

#ifdef DO_BZERO
	bzero(ret->start, ret->totalsize);
#endif

	/* 
	 * walk the buffers initializing the free list.
	 */

	for (ptr = ret->start;
	     ptr < ret->end;
	     ptr += size)
	{
		gen = ptr;
		gen->next = ptr + size;
		gen->magic = MPOOLBUFMAGIC;
	}

	gen = ptr;
	gen->next = NULL;
	gen->magic = MPOOLBUFMAGIC;

	ret->free = numelements;
	ret->head = ret->start;
	ret->poolmagic = MPOOLMAGIC;

	return ret;
}

int
pool_init_estimate(size, numelements)
	int size;
	int numelements;
{
	int totalsize;

	size = ((size + 7) & ~7);

	totalsize = sizeof(mpool);
	totalsize += size * numelements;
	totalsize += (numelements / 32 + 1) * 4;   /* for the bitmap */
	totalsize += 32;  /* for safety */

	return totalsize;
}

/*
 * don't call this function if you're using your own
 * allocate function. must clean up yourself.
 */

void
pool_destroy(mp)
	mpool *mp;
{
	if (validate_mp(mp, "pool_destroy") == 0)
	{
		free(mp);
	}
}

void *
pool_alloc(mp)
	mpool *mp;
{
	struct mpool_data *new, *nxt;
	int buf, bit, entry;

	if (validate_mp(mp, "pool_alloc"))
		return NULL;

	new = mp->head;

	if (new == NULL || mp->free == 0)
	{
		fprintf(stderr, "pool_alloc: no buffers free in this pool\n");
		return NULL;
	}

	if (new->magic != MPOOLBUFMAGIC)
	{
		fprintf(stderr, "pool_alloc: magic number mismatch\n");
		return NULL;
	}

	buf = ((void*)new - mp->start) / mp->size;

	CALCBITNUMBER(buf, bit, entry);

	if (mp->bitmap[entry] & (1 << bit))
	{
		fprintf(stderr, 
			"pool_alloc: bitmap says buf already allocated!\n");
		return NULL;
	}

	nxt = new->next;

	if (nxt == NULL)
	{
		if (mp->free > 1)
		{
			fprintf(stderr, 
				"pool_alloc: suspect "
			       "freelist corruption (case 1)\n");
			return NULL;
		}
	} else {
		if (((void*)nxt < mp->start) || 
		    ((void*)nxt > mp->end))
		{
			fprintf(stderr, 
				"pool_alloc: suspect "
			       "freelist corruption (case 2)\n");
			return NULL;
		}
		if ((((void*)nxt - mp->start) % mp->size) != 0)
		{
			fprintf(stderr,
				"pool_alloc: suspect "
			       "freelist corruption (case 3)\n");
			return NULL;
		}
		if (nxt->magic != MPOOLBUFMAGIC)
		{
			fprintf(stderr,
				"pool_alloc: suspect "
			       "freelist corruption (case 4)\n");
			return NULL;
		}
	}

	mp->bitmap[entry] |= (1 << bit);

	mp->head = nxt;
#ifdef DO_BZERO
	bzero(new, mp->size);
#else
	/* nuke the magic number. */
	new->magic = 0;
#endif

	mp->free--;

	return new;
}

void
pool_free(mp, dat)
	mpool *mp;
	void *dat;
{
	struct mpool_data *new;
	void *oldhead;
	int buf, bit, entry;

	if (validate_mp (mp,  "pool_free") ||
	    validate_dat(dat, "pool_free"))
		return;

	if (dat < mp->start || dat > mp->end)
	{
		fprintf(stderr, 
			"pool_free: dat doesn't belong to this pool\n");
		return;
	}

	if (((dat - mp->start) % mp->size) != 0)
	{
		fprintf(stderr, 
			"pool_free: dat isn't on a buffer boundary\n");
		return;
	}

	buf = (dat - mp->start) / mp->size;

	CALCBITNUMBER(buf, bit, entry);

	if ((mp->bitmap[entry] & (1 << bit)) == 0)
	{
		fprintf(stderr, "pool_free: buffer is already free\n");
		return;
	}

	mp->bitmap[entry] &= ~(1 << bit);

	new = dat;

	if (new->magic == MPOOLBUFMAGIC)
		fprintf(stderr, "pool_free: is this buffer already free?\n");

#ifdef DO_BZERO
	bzero(dat, mp->size);
#endif

	new->magic = MPOOLBUFMAGIC;
	oldhead = mp->head;
	mp->head = new;
	new->next = oldhead;

	mp->free++;
}

void
pool_dump(mp)
	mpool *mp;
{
	int i, total;

	if ( validate_mp( mp, "pool_dump" ))
		return;

	total = mp->totalsize / mp->size;

	printf( "pool %#x:  %d bytes %d buffers %d free  freelist:\n",
		mp, mp->size, total, mp->free );

	for ( i = 0; i < ((total / 32) + 1); i++ )
	{
		printf( "%08x", mp->bitmap[i] );
	}

	printf( "\n" );
}
