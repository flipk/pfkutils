#if 0
files="FileBlockLocal.C FileBlockLocalCache.C"
set -e
set -x
g++ -I../h -I../dll2 -Wall -Werror $files main.C ../dll2/dll2_hash.C
./a.out
exit 0
#endif

#include <stdio.h>

#include "FileBlockLocal.H"

int
main()
{
    FileBlockInterface * fbi;
    fbi = new FileBlockLocal( "testfile.db", true );

    delete fbi;
    return 0;
}
