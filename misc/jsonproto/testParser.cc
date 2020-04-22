
#include "jsonproto.h"
#include <stdio.h>

int
main()
{
    JsonProtoConverter  cvt1, cvt2;

    std::vector<std::string> searchPath;

    searchPath.push_back("../protobuf");

    if (cvt2.parseProto("../protobuf/test2.proto",
                       &searchPath) == false)
    {
        printf("cannot parse test2.proto\n");
        return 1;
    }

    if (cvt2.emitJsonProtoConverter() == false)
    {
        printf("cannot output converter source 2\n");
        return 1;
    }

    if (cvt1.parseProto("../protobuf/test1.proto",
                       &searchPath) == false)
    {
        printf("cannot parse test1.proto\n");
        return 1;
    }

    if (cvt1.emitJsonProtoConverter() == false)
    {
        printf("cannot output converter source 1\n");
        return 1;
    }

    printf("done producing converter\n");

    return 0;
}
