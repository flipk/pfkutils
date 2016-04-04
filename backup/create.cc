/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include "database_items.h"
#include <iostream>
using namespace std;

void
bakFile::create(void)
{
    if (createFiles() == false)
        return;

    {
        bakDatum  dat(bt);
        dat.key_dbinfo();
        dat.data.dbinfo.sourcedir() = opts.sourcedir;
        dat.data.dbinfo.nextver() = 1;
        dat.mark_dirty();
    } // bakDatum destructor puts datum

    if (opts.verbose > 0)
        cout << "database created and initialized\n";

    _update();
}
