
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file FileBlockLocalValidate.C
 * \brief Validation and debug functions.
 * \author Phillip F Knaack
 */

#include "FileBlockLocal.H"

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
        printf("  used aus: %d  free aus: %d  first au: %d\n"
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
                printf("  auid %d = aun %d\n", auid, aun);
        }
        for (aun=0; aun < fh.d->info.auid_stack_top.get(); aun++)
        {
            auid = lookup_stack(aun);
            printf( "  free stack ind %d -> auid %d\n", aun, auid);
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
            printf("ERROR: could not fetch au %d\n", aun);
            return;
        }
        int au_size = au.d->size();
        if (verbose)
        {
            printf("  %d: %s ", aun,
                   au.d->used() ? "USED" : "FREE");
            if (au_size != 0)
                printf("size %d ", au_size);
            else
                printf("LAST ");
            printf("prev %d ", au.d->prev.get());
            if (au.d->used())
                printf("auid %d", au.d->auid());
            else
                printf("bucket_next %d bucket_prev %d",
                       au.d->bucket_next(),
                       au.d->bucket_prev());
            printf("\n");
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
                printf("%d ", aun);
            AUHead  au(bc);
            if (!au.get(aun))
            {
                printf("ERROR: unable to get free aun %d\n", aun);
                return;
            }
            if (au.d->used() == true)
            {
                printf("ERROR: aun %d used but on bucket list %d!\n", aun, i);
                return;
            }
            tmp = au.d->bucket_prev();
            if (prev_aun != tmp)
            {
                printf("ERROR: aun %d has incorrect bucket_prev (%d!=%d)\n",
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
