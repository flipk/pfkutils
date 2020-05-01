#ifndef __JSONPROTO_H__
#define __JSONPROTO_H__ 1

#include <string>
#include <vector>
#include "protobuf_tokenize_and_parse.h"

class JsonProtoConverter
{
    ProtoFile * protoFile;
public:
    JsonProtoConverter(void);
    ~JsonProtoConverter(void);
    bool parseProto(const std::string &fname,
                    const std::vector<std::string> *searchPath);
    bool emitJsonProtoConverter(std::string &out_h_fname,
                                std::string &out_cc_fname);
};

#endif /* __JSONPROTO_H__ */

