#if 0
set -e -x
g++ -DINCLUDE_TEST_MAIN -Wall -Werror asm_parser.cc -o asm_parser
./asm_parser dis pc.list
exit 0
;
#endif

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "asm_parser.h"
#include "simpleRegex.h"

class asmlineparser : public regex<>
{
    static const char * pattern;
public:
    asmlineparser(void) : regex<>(pattern) { }
    ~asmlineparser(void) { /*nothing*/ }
    enum groups {
        FUNCADDR = 1,
        FUNCNAME,
        PCADDR,
        PCREST,
        WILDCARD
    };
};

const char * asmlineparser :: pattern = 
    "^(0[0-9a-f]+) <(.*)>:.*$|"   // FUNCADDR, FUNCNAME
    "^ +([0-9a-f]+):[ \t]*(.*)$|" // PCADDR,   PCREST
    "^(.*)$";                     // WILDCARD

asm_parser :: asm_parser(bool _debug /*= false*/)
    : debug(_debug)
{
}

asm_parser :: ~asm_parser(void)
{
    pc_map_t::iterator it1;
    while ((it1 = pc_map.begin()) != pc_map.end())
    {
        delete it1->second;
        pc_map.erase(it1);
    }
    func_map_t::iterator it2;
    while ((it2 = func_map.begin()) != func_map.end())
    {
        delete it2->second;
        func_map.erase(it2);
    }
}

bool
asm_parser :: parse_file(const std::string &fname)
{
    asmlineparser   p;
    std::ifstream   f;

    f.open(fname.c_str());
    if (f.fail())
    {
        char * e = strerror(errno);
        fprintf(stderr, "unable to open file '%s': '%s'\n",
                fname.c_str(), e);
        return false;
    }

    std::string l;
    func_desc * current_func = NULL;
    pc_desc * prev_pc = NULL;

    while (f.good())
    {
        std::getline(f, l);
        if (!f.good())
            break;
        if (p.exec(l) == false)
            printf(" failed to parse line: '%s'\n", l.c_str());
        if (p.match(asmlineparser::FUNCADDR))
        {
            current_func = new func_desc;
            // no strtoul error checking because if regex matched,
            // strtoul will work.
            current_func->addr = strtoul(
                p.match(l,
                        asmlineparser::FUNCADDR).c_str(),
                NULL, 16);
            current_func->func = p.match(l, asmlineparser::FUNCNAME);
            func_map[current_func->addr] = current_func;
            if (debug)
                printf("adding func '%s' at 0x%x\n",
                       current_func->func.c_str(),
                       current_func->addr);
        }
        else if (p.match(asmlineparser::PCADDR))
        {
            unsigned int addr =
                strtoul(
                    p.match(l,
                            asmlineparser::PCADDR).c_str(),
                    NULL, 16);
            if (current_func != NULL)
            {
                pc_desc * pc = new pc_desc;
                pc->addr = addr;
                pc->line = p.match(l, asmlineparser::PCREST);
                pc->func = current_func;
                pc->prev = prev_pc;
                pc_map[pc->addr] = pc;
                if (debug)
                    printf("adding pc 0x%x in func '%s'\n",
                           pc->addr, pc->func->func.c_str());
                prev_pc = pc;
            }
            else
            {
                if (debug)
                    printf("pc 0x%x is not registered to a function\n", addr);
            }
        }
        else if (p.match(asmlineparser::WILDCARD))
        {
            // skip
        }
    }    
    return true;
}

#ifdef INCLUDE_TEST_MAIN

int
main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("usage: asm_parser <disassembly> <list_of_pcs>\n");
        return 1;
    }

    bool debug = getenv("DEBUG") != NULL;

    asm_parser   p(debug);

    if (p.parse_file(argv[1]) == false)
    {
        printf("error parsing disassembly file\n");
        return 1;
    }

    std::ifstream  ifs;

    ifs.open(argv[2]);
    if (!ifs.good())
    {
        int e = errno;
        char * err = strerror(e);
        printf("open input '%s': %d (%s)\n", argv[2], e, err);
        return 1;
    }

    std::string l;

    while (ifs.good())
    {
        std::getline(ifs, l);
        if (!ifs.good())
            break;
        unsigned int addr = strtoul(l.c_str(), NULL, 16);
        asm_parser::pc_map_t::iterator it = p.pc_map.find(addr);
        if (it != p.pc_map.end())
        {
            asm_parser::pc_desc * pc = it->second->prev;
            printf("%s <%s>: %s\n",
                   l.c_str(),
                   pc->func->func.c_str(),
                   pc->line.c_str());
        }
    }

    return 0;
}

#endif
