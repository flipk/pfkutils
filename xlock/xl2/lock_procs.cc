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

#include "xl2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

int LockProc::factory_count = 0;
LockProcFactory * LockProc::factories[LockProc::max_factories];

//static
LockProc *
LockProc :: make(const std::string &name, xl2_screen &s)
{
    for (int ind = 0; ind < factory_count; ind++)
    {
        LockProcFactory * lpf = factories[ind];
        if (strcmp(name.c_str(), lpf->name) == 0)
            return lpf->make(s);
    }
    printf("lock proc '%s' not found\n", name.c_str());
    return NULL;
}

//static
LockProc *
LockProc :: make_random(xl2_screen &s)
{
    int ind = (random() & 0x7fffffff) % factory_count;
    LockProcFactory * lpf = factories[ind];
    return lpf->make(s);
}

//static
void
LockProc :: register_lp(LockProcFactory * lpf)
{
    if (factory_count >= max_factories)
    {
        fprintf(stderr, "maximum # factories reached!!\n");
        return;
    }
    factories[factory_count++] = lpf;
}
