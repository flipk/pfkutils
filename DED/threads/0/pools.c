/*
 * functions for management of pool SETs.
 *
 * interfaces with the pool_* interface to provide
 * a set of different-sized pools; allocations are 
 * matched to the optimal pool size, and frees
 * are freed to the correct pool.
 */

#include "internal.h"

__inline static int
validate_mps(mps, name)
	mpoolset *mps;
	char *name;
{
	if (mps == NULL)
	{
		fprintf(stderr, "%s: null cookie passed in\n", name);
		return 1;
	}

	if (mps->poolsetmagic != MPOOLSETMAGIC)
	{
		fprintf(stderr, "%s: poolset magic number mismatch\n", name);
		return 1;
	}

	return 0;
}

struct _quantsiz {
	int quantity;
	int size;
	int totalsize;
};

static int
quantsiz_compare(a, b)
	const struct _quantsiz *a;
	const struct _quantsiz *b;
{
	if (a == NULL)
		return -1;
	if (b == NULL)
		return 1;
	if (a->size == b->size)
		return 0;
	if (a->size < b->size)
		return -1;
	return 1;
}

mpoolset *
poolset_init(numpools, quantities, sizes)
	int numpools;
	int *quantities;
	int *sizes;
{
	mpoolset *ret;
	struct _quantsiz *quantsizes;
	int i;
	int structsize, totalsize;
	void *nextpool;

	if (numpools < 1)
	{
		fprintf(stderr,
			"poolset_init: invalid numpools %d\n", numpools);
		return NULL;
	}

	/*
	 * numpools minus 1 because the struct has one in the defn
	 * already, and gdb doesn't like having zero-sized arrays.
	 * it does work if you're not planning on using gdb, however.
	 */

	structsize = totalsize = sizeof(mpoolset) + 
		sizeof(struct mpool *) * (numpools - 1);

	/* 
	 * set up a temporary array to hold the quantities and
	 * sizes.  we want to sort them from smallest to largest,
	 * but we don't want to modify the user's buffer.
	 */

	quantsizes = (struct _quantsiz *)
		malloc(sizeof(struct _quantsiz) * numpools);

	if (quantsizes == NULL)
	{
		fprintf(stderr, 
			"poolset_init: malloc for temp space failed\n");
		return NULL;
	}

	for (i=0; i < numpools; i++)
	{
		quantsizes[i].quantity = quantities[i];
		quantsizes[i].size     = sizes[i];
	}

	qsort(quantsizes, numpools, sizeof(struct _quantsiz),
	      (int (*)(const void *, const void *))quantsiz_compare);

	/*
	 * calculate total memory needed for poolset.
	 */

	for (totalsize = i = 0; i < numpools; i++)
	{
		register int sz;

		sz = pool_init_estimate(quantsizes[i].size, 
					quantsizes[i].quantity);
		quantsizes[i].totalsize = sz;
		totalsize += sz;
	}

	ret = malloc(totalsize);
	if (ret == NULL)
	{
		fprintf(stderr, "poolset_init: malloc failed for poolset\n");
		free(quantsizes);
		return NULL;
	}

	nextpool = (void*)ret + structsize;
	ret->numpools = numpools;

	/*
	 * now initialize each pool from the sorted list.
	 * if any one allocation fails, delete all other pools
	 * and abort the operation.
	 */

	for (i=0; i < numpools; i++)
	{
		ret->pools[i] = _pool_init(quantsizes[i].size,
					   quantsizes[i].quantity,
					   nextpool);

		if (ret->pools[i] == NULL)
		{
			fprintf(stderr, 
				"poolset_init: pool %d "
				"(size = %d, quant = %d) init failed\n",
				quantsizes[i].size, quantsizes[i].quantity);
			free(ret);
			free(quantsizes);
			return NULL;
		}

		nextpool += quantsizes[i].totalsize;
	}

	/*
	 * for efficient bounds checking on the buffer sizes,
	 * we keep around the size of the largest buffer the
	 * user wants.  in our sorted array, this size is the last one.
	 */

	ret->maxbufsize = quantsizes[numpools-1].size;
	free(quantsizes);
	ret->poolsetmagic = MPOOLSETMAGIC;

	return ret;
}

void
poolset_destroy(mps)
	mpoolset *mps;
{
	/*
	 * since there was only one malloc, there
	 * needs to be only one free.
	 */

	if (validate_mps(mps, "poolset_destroy") == 0)
	{
		free(mps);
	}
}

void *
poolset_alloc(mps, size)
	mpoolset *mps;
	int size;
{
	int i;

	if (validate_mps(mps, "poolset_alloc"))
		return NULL;

	if (size <= 0)
	{
		fprintf(stderr, 
			"poolset_alloc: requested size %d invalid\n", size);
		return NULL;
	}

	if (size > mps->maxbufsize)
	{
		fprintf(stderr, 
			"poolset_alloc: request larger than largest "
			"buffer in set\n");
		return NULL;
	}

	/* 
	 * walk the list of pools, finding one that has both
	 * a size large enough to satisfy the request and a free
	 * buffer in its pool.
	 * i.e. if a buffer of size 20 is requested, and the pool
	 * with 20-byte buffers is empty, it will go up to the next
	 * larger pool.
	 */

	for (i=0; i < mps->numpools; i++)
	{
		if ((size <= mps->pools[i]->size) &&
		    (mps->pools[i]->free > 0))
		{
			return pool_alloc(mps->pools[i]);
		}
	}

	/*
	 * we only drop out of the for loop if we've walked
	 * all the way to the end of the poolset and couldn't find
	 * a match.
	 */

	fprintf(stderr, "poolset_alloc: no buffers available for this size\n");

	poolset_dump( mps );

	return NULL;
}

void
poolset_free(mps, dat)
	mpoolset *mps;
	void *dat;
{
	int i;

	if (validate_mps(mps, "poolset_free"))
		return;

	/*
	 * all addresses are linear as we move forward 
	 * in the poolset, so a simple address comparison will
	 * tell us which pool the buffer is in.
	 */

	if (dat < mps->pools[0]->start)
	{
		fprintf(stderr, 
			"poolset_free: address too low for this poolset\n");
		return;
	}

	for (i=0; i < mps->numpools; i++)
	{
		if (dat <= mps->pools[i]->end)
			break;
	}

	if (i == mps->numpools)
	{
		fprintf(stderr, 
			"poolset_free: address too high for this poolset\n");
		return;
	}

	pool_free(mps->pools[i], dat);
}

void
poolset_dump(mps)
	mpoolset * mps;
{
	int i;

	printf( "poolset %#x: %d pools, maxbufsize=%d; pools:\n", 
		mps, mps->numpools, mps->maxbufsize );
	for ( i = 0; i < mps->numpools; i++ )
		pool_dump( mps->pools[i] );
}
