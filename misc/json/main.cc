
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "simple_json.h"
#include "tokenize_and_parse.h"

static void callback(void *arg, const std::string &buf)
{
    printf("callback : '%s'\n", buf.c_str());
    SimpleJson::ObjectProperty * obj = SimpleJson::parseJson(buf);
    std::cout << obj << std::endl;
    delete obj;
}

int
main(int argc, char ** argv)
{
    char buf[100];
    FILE * f = fopen(argv[1], "r");
    SimpleJson::SimpleJsonCollector   sjc(callback, NULL);
    if (f)
    {
        char * dbg = getenv("DBG");
        if (dbg != NULL)
        {
            json_parser_debug_tokenize(f);
        }
        else
        {
            int cc;
            while ((cc = fread(buf, 1, sizeof(buf), f)) > 0)
            {
                int pos = 0, remain = cc;
                while (remain > 0)
                {
                    int added = sjc.add_data(buf + pos, remain);
                    printf("add_data returns %d\n", added);
                    remain -= added;
                    pos += added;
                }
            }
        }
        fclose(f);
    }
    return 0;
}
