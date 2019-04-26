
#include <stdio.h>
#include "parser.h"

int main()
{
    csvline * lines = parse_file(stdin);

    for (auto l = lines; l; l = l->next)
    {
        printf("LINE: (");
        for (auto f = l->fields; f; f = f->next)
        {
            printf("'%s'", f->word.c_str());
            if (f->next)
                printf(",");
        }
        printf(")\n");
    }

    delete lines;

    return 0;
}
