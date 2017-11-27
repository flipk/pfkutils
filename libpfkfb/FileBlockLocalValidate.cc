/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file FileBlockLocalValidate.cc
 * \brief Validation and debug functions.
 */

//redhat needs this for PRIu64 and friends.
#define __STDC_FORMAT_MACROS 1

#include "FileBlockLocal.h"

#include <stdlib.h>

void
FileBlockLocal :: validate( bool verbose )
{
    int i;
    FB_AUN_T  aun;
    FB_AUID_T auid;

    if (fh.d->info.signature.get() != InfoBlock::SIGNATURE)
    {
        printf("ERROR: bad signature!\n");
        return;
    }
    if (verbose)
    {
        printf("  good signature\n");
        printf("  used aus: %d  free aus: %d  first au: %" PRIu64 "\n"
               "  num_aus: %d used ext: %d  free ext: %d\n",
               fh.d->info.used_aus.get(),
               fh.d->info.free_aus.get(),
               fh.d->info.first_au.get(),
               fh.d->info.num_aus.get(),
               fh.d->info.used_extents.get(),
               fh.d->info.free_extents.get());
        printf("  auid_top: %d  auid_stack_top: %d\n",
               fh.d->info.auid_top.get(),
               fh.d->info.auid_stack_top.get());
        for (auid=0; auid < fh.d->info.auid_top.get(); auid++)
        {
            aun = translate_auid(auid);
            if (aun != 0)
                printf("  auid %d = aun %" PRIu64 "\n", auid, aun);
        }
        for (aun=0; aun < fh.d->info.auid_stack_top.get(); aun++)
        {
            auid = lookup_stack(aun);
            printf( "  free stack ind %" PRIu64 " -> auid %d\n", aun, auid);
        }
    }

    DataInfoBlock dib(this);

    for (i=0; i < DataInfoPtrs::MAX_DATA_INFOS; i++)
    {
        FB_AUID_T dip = fh.d->data_info_ptrs.ptrs[i].get();
        if (dip == 0)
            continue;
        dib.get(dip);
        if (verbose)
            printf("  dip %d: '%s' -> %d\n",
                   i, dib.d->info_name, dib.d->info_auid.get());
    }
    dib.release();

    BucketList   * bl = &fh.d->bucket_list;
    BucketBitmap * bm = &fh.d->bucket_bitmap;

    aun = fh.d->info.first_au.get();
    while (true)
    {
        AUHead  au(bc);
        if (!au.get(aun))
        {
            printf("ERROR: could not fetch aun %" PRIu64 "\n", aun);
            return;
        }
        int au_size = au.d->size();
        if (verbose)
        {
            printf("  %" PRIu64 ": %s ", aun,
                   au.d->used() ? "USED" : "FREE");
            if (au_size != 0)
                printf("size %d ", au_size);
            else
                printf("LAST ");
            printf("prev %" PRIu64 " ", au.d->prev.get());
            if (au.d->used())
                printf("auid %d", au.d->auid());
            else
                printf("bucket_next %" PRIu64 " bucket_prev %" PRIu64,
                       au.d->bucket_next(),
                       au.d->bucket_prev());
            printf("\n");
            if (au.d->used() && au.d->auid() != 0)
            {
                FileBlock * fb = get(au.d->auid());
                uint8_t * ptr = fb->get_ptr();
                int size = fb->get_size();
                printf("  contents:\n");
                for (int pos = 0; pos < size; pos++)
                {
                    if ((pos & 31) == 0)
                        printf("    ");
                    printf("%02x", ptr[pos]);
                    if ((pos & 3) == 3)
                        printf(" ");
                    if ((pos & 7) == 7)
                        printf(" ");
                    if ((pos & 31) == 31)
                        printf("\n");
                }
                printf("\n");
                release(fb);
            }
        }
        if (au_size == 0)
            break;
        aun += au_size;
    }

    for (i=0; i < BucketList::NUM_BUCKETS; i++)
    {
        FB_AUN_T aun = bl->list_head[i].get();
        bool used = bm->get_bit(i);
        if (aun == 0 && used == false)
            continue;
        if (aun == 0 && used == true)
        {
            printf("ERROR: bucket %d bit set but list empty!\n", i);
            return;
        }
        if (aun != 0 && used == false)
        {
            printf("ERROR: bucket %d bit clear but list not empty!\n", i);
            return;
        }
        if (verbose)
            printf("  bucket %d: ", i);
        FB_AUN_T tmp, prev_aun = 0;
        while (aun != 0)
        {
            if (verbose)
                printf("%" PRIu64 " ", aun);
            AUHead  au(bc);
            if (!au.get(aun))
            {
                printf("ERROR: unable to get free aun %" PRIu64 "\n", aun);
                return;
            }
            if (au.d->used() == true)
            {
                printf("ERROR: aun %" PRIu64 " used but on "
                       "bucket list %d!\n", aun, i);
                return;
            }
            tmp = au.d->bucket_prev();
            if (prev_aun != tmp)
            {
                printf("ERROR: aun %" PRIu64 " has incorrect "
                       "bucket_prev (%" PRIu64 "!=%" PRIu64 ")\n",
                       aun, prev_aun, tmp);
                return;
            }
            prev_aun = aun;
            aun = au.d->bucket_next();
        }
        if (verbose)
            printf("\n");
    }
}
