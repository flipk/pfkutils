
#include "jsonproto.h"
#include <stdio.h>
#include <stdlib.h>

void
usage(void)
{
    fprintf(stderr, "usage: makeJsonProtoConverter "
            "path[:path...] filename.proto file.h file.cc\n"
            "   specify a list of paths where proto files may reside\n"
            "    (for resolving imports)\n"
            "   then specify one proto file to process.\n");
    exit(1);
}

int
main(int argc, char ** argv)
{
    if (argc != 5)
    {
        usage();
        return 1;
    }

    std::string path      = argv[1];
    std::string protofile = argv[2];
    std::string hfile     = argv[3];
    std::string ccfile    = argv[4];

    std::vector<std::string> searchPath;
    for (size_t pos = 0; ;)
    {
        size_t found = path.find_first_of(":",pos);
        if (found == std::string::npos)
        {
            if (pos != path.size())
                searchPath.push_back(path.substr(pos,path.size()-pos));
            break;
        }
        if (found > pos)
            searchPath.push_back(path.substr(pos,found-pos));
        pos = found+1;
    }

    JsonProtoConverter  cvt;

    if (cvt.parseProto(protofile, &searchPath) == false)
    {
        printf("cannot parse %s\n", protofile.c_str());
        return 1;
    }

    if (cvt.emitJsonProtoConverter(hfile, ccfile) == false)
    {
        printf("cannot output converter source\n");
        return 1;
    }

    printf("done producing converter\n");

    return 0;
}
