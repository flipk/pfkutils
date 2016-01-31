/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include "database_items.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>

using namespace std;

void
bakFile::export_tar (void)
{
    int tarfd = ::open(opts.tarfile.c_str(),
                       O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (tarfd < 0)
    {
        int e = errno;
        cerr << "can't open tar file " << opts.tarfile << ": "
             << strerror(e) << endl;
        return;
    }

    _extract(tarfd);
}
