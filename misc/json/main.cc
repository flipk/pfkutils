
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "simple_json.h"

// for debug_tokenize only; not needed for SimpleJson
#include "json_tokenize_and_parse.h"

static void callback(void *arg, const std::string &buf)
{
    printf("callback : '%s'\n", buf.c_str());
    SimpleJson::Property * obj = SimpleJson::parseJson(buf);
    std::cout << obj << std::endl;
    delete obj;
}

int
main(int argc, char ** argv)
{
    if (getenv("STRING_ONLY") == NULL)
    {
        char buf[100];
        FILE * f = fopen(argv[1], "re");
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
    }
    else
    {
        SimpleJson::Property * obj;

        obj = SimpleJson::parseJson("\"just a string\"");
        std::cout << obj << std::endl;
        delete obj;

        obj = SimpleJson::parseJson("12345");
        std::cout << obj << std::endl;
        delete obj;

        obj = SimpleJson::parseJson("123.45");
        std::cout << obj << std::endl;
        delete obj;
    }
    return 0;
}
