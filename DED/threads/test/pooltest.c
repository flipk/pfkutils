#include <stdio.h>

#include "th.h"

int n = 4;
int q[] = { 5, 5, 5, 5 };
int s[] = { 10, 20, 30, 40 };

int
pooltest_main()
{
	mpoolset *mps;
	void *ptr[50];
	int i,j;

	printf("expect eight of these: \"no buffers "
	       "available for this size\"\n");

	mps = poolset_init(n, q, s);

	for (j=i=0; i < 6; i++)
	{
		ptr[j++] = poolset_alloc(mps, 10);
		ptr[j++] = poolset_alloc(mps, 20);
		ptr[j++] = poolset_alloc(mps, 30);
		ptr[j++] = poolset_alloc(mps, 40);
	}

	for (i=0; i < j; i++)
	{
		if (ptr[i] != NULL)
			poolset_free(mps, ptr[i]);
	}

	for (j=i=0; i < 6; i++)
	{
		ptr[j++] = poolset_alloc(mps, 10);
		ptr[j++] = poolset_alloc(mps, 20);
		ptr[j++] = poolset_alloc(mps, 30);
		ptr[j++] = poolset_alloc(mps, 40);
	}

	for (i=0; i < j; i++)
	{
		if (ptr[i] != NULL)
			poolset_free(mps, ptr[i]);
	}

	return 0;
}
