
#include "jsonproto.h"
#include "template_patterns.h"

#ifndef DEPENDING
#include "converter_template.h"
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctype.h>

using namespace std;

JsonProtoConverter :: JsonProtoConverter(void)
{
    protoFile = NULL;
}

JsonProtoConverter :: ~JsonProtoConverter(void)
{
    if (protoFile)
        delete protoFile;
}

bool
JsonProtoConverter :: parseProto(const string &fname,
                                 const vector<string> *searchPath)
{
    protoFile = protobuf_parser(fname, searchPath);
    if (protoFile == NULL)
        return false;
    return true;
}

static inline string
dots_to_something(const string &in, const string &something)
{
    string out;
    for (size_t ind = 0; ind < in.size(); ind++)
    {
        char c = in[ind];
        if (c == '.')
            out += something;
        else
            out += c;
    }
    return out;
}

static inline string
lower_case(const string &in)
{
    string out;
    for (size_t ind = 0; ind < in.size(); ind++)
    {
        char c = in[ind];
        if (isupper(c))
            c = tolower(c);
        out += c;
    }
    return out;
}

static void
emitOneMessage(pattern_value_map  &patterns,
               ofstream &out_h, ofstream &out_cc,
               ProtoFileMessage *m)
{
        for (ProtoFileMessage * m2 = m->sub_messages; m2; m2 = m2->next)
            emitOneMessage(patterns, out_h, out_cc, m2);

        ostringstream json2proto_msgsrcbody, proto2json_msgsrcbody;
        bool proto2json_failbody = false;

        patterns["msgtype"] = m->fullname();
        output_HEADER_message(out_h, patterns);
        for (ProtoFileMessageField * f = m->fields; f; f = f->next)
        {
            patterns["fieldname"] = f->name;
            patterns["fieldnamelower"] = lower_case(f->name);
            if (f->external_package)
            {
                patterns["fieldpkg"] =
                    dots_to_something(f->type_package, "::") + "::";
            }
            else
            {
                patterns["fieldpkg"] = "";
            }
            if (f->typetype == ProtoFileMessageField::MSG)
                patterns["fieldtype"] = f->type_msg->fullname();
            else
                patterns["fieldtype"] = f->type_name;
            if (f->occur == ProtoFileMessageField::REPEATED)
            {
                output_SOURCE_msg_json2proto_repeated_start(
                    json2proto_msgsrcbody, patterns);
                output_SOURCE_msg_proto2json_repeated_start(
                    proto2json_msgsrcbody, patterns);
                switch (f->typetype)
                {
                case ProtoFileMessageField::ENUM:
                    proto2json_failbody = true;
                    output_SOURCE_msg_proto2json_enum_repeated(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_enum_repeated(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::MSG:
                    proto2json_failbody = true;
                    output_SOURCE_msg_proto2json_msg_repeated(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_msg_repeated(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::BOOL:
                    output_SOURCE_msg_proto2json_bool_repeated(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_bool_repeated(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::STRING:
                    output_SOURCE_msg_proto2json_string_repeated(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_string_repeated(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::UNARY:
                    output_SOURCE_msg_proto2json_unary_repeated(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_unary_repeated(
                        json2proto_msgsrcbody, patterns);
                    break;
                }
                output_SOURCE_msg_json2proto_repeated_end(
                    json2proto_msgsrcbody, patterns);
                output_SOURCE_msg_proto2json_repeated_end(
                    proto2json_msgsrcbody, patterns);
            }
            else // occur = optional or required
            {
                output_SOURCE_msg_proto2json_start(
                    proto2json_msgsrcbody, patterns);
                output_SOURCE_msg_json2proto_start(
                    json2proto_msgsrcbody, patterns);
                switch (f->typetype)
                {
                case ProtoFileMessageField::ENUM:
                    proto2json_failbody = true;
                    output_SOURCE_msg_proto2json_enum(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_enum(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::STRING:
                    output_SOURCE_msg_proto2json_string(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_string(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::MSG:
                    proto2json_failbody = true;
                    output_SOURCE_msg_proto2json_msg(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_msg(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::BOOL:
                    output_SOURCE_msg_proto2json_bool(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_bool(
                        json2proto_msgsrcbody, patterns);
                    break;
                case ProtoFileMessageField::UNARY:
                    output_SOURCE_msg_proto2json_unary(
                        proto2json_msgsrcbody, patterns);
                    output_SOURCE_msg_json2proto_unary(
                        json2proto_msgsrcbody, patterns);
                    break;
                }
                output_SOURCE_msg_proto2json_end(
                    proto2json_msgsrcbody, patterns);
                output_SOURCE_msg_json2proto_end(
                    json2proto_msgsrcbody, patterns);
            }
        }

        if (proto2json_failbody)
        {
            output_SOURCE_message_failbody(
                proto2json_msgsrcbody, patterns);
        }

        patterns["json2proto_msgsrcbody"] = json2proto_msgsrcbody.str();
        patterns["proto2json_msgsrcbody"] = proto2json_msgsrcbody.str();
        output_SOURCE_message(out_cc, patterns);
}


bool
JsonProtoConverter :: emitJsonProtoConverter(std::string &out_h_fname,
                                             std::string &out_cc_fname)
{
    if (protoFile == NULL)
        return false;

    size_t protosuffixpos = protoFile->filename.find(".proto");

    if (out_h_fname == "")
        out_h_fname = protoFile->filename.substr(0,protosuffixpos)
            + "_converter.h";
    if (out_cc_fname == "")
        out_cc_fname = protoFile->filename.substr(0,protosuffixpos)
            + "_converter.cc";

    ofstream  out_h(out_h_fname.c_str(), ios_base::out | ios_base::trunc);
    ofstream out_cc(out_cc_fname.c_str(), ios_base::out | ios_base::trunc);

    if (!out_h.good() || !out_cc.good())
    {
        cerr << "FAIL : unable to open output\n";
        return false;
    }

    pattern_value_map  patterns;
    string opennamespaces, closenamespaces, package;

    package = dots_to_something(protoFile->package, "::");

    {
        string namespace_name;
        for (size_t ind = 0; ind < protoFile->package.size(); ind++)
        {
            char c = protoFile->package[ind];
            if (c == '.')
            {
                opennamespaces += "namespace " + namespace_name + " {\n";
                closenamespaces += "} // namespace " + namespace_name + "\n";
                namespace_name = "";
            }
            else
                namespace_name += c;
        }
        opennamespaces += "namespace " + namespace_name + " {\n";
        closenamespaces += "} // namespace " + namespace_name + "\n";
    }

    size_t startpos = out_h_fname.find_last_of('/');
    if (startpos == string::npos)
        startpos = 0;
    else
        startpos ++; // skip slash
    size_t dotpos = out_h_fname.find_last_of('.');
    string out_h_fname_nodot = out_h_fname.substr(startpos,dotpos-startpos);

    {
        ostringstream pbincludes;
        out_h << "//\n"
              << "// this is autogenerated from: "
              << protoFile->filename;
        protosuffixpos = protoFile->filename.find(".proto");
        pbincludes << "#include \""
                   << protoFile->filename.substr(0,protosuffixpos) << ".pb.h"
                   << "\"\n";
        for (size_t ind = 0; ind < protoFile->import_filenames.size(); ind++)
        {
            const string &fn = protoFile->import_filenames[ind];
            protosuffixpos = fn.find(".proto");
            pbincludes << "#include \""
                       << fn.substr(0,protosuffixpos) << ".pb.h"
                       << "\"\n";
            pbincludes << "#include \""
                       << fn.substr(0,protosuffixpos) << "_converter.h"
                       << "\"\n";
            out_h << " " << fn;
        }
        out_h << "\n//\n\n";
        patterns["pbincludes"] = pbincludes.str();
    }

    patterns["opennamespaces"] = opennamespaces;
    patterns["closenamespaces"] = closenamespaces;
    patterns["headerfilename"] = out_h_fname;
    patterns["headerfilenamenodot"] = out_h_fname_nodot;
    patterns["package"] = package;

    output_HEADER_top(out_h, patterns);
    output_SOURCE_top(out_cc, patterns);

    for (ProtoFileEnum * e = protoFile->enums; e; e = e->next)
    {
        ostringstream json2proto_enumsrcbody, proto2json_enumsrcbody;
        patterns["fieldtype"] = e->name;
        output_HEADER_enum(out_h, patterns);
        bool first = true;
        for (ProtoFileEnumValue * v = e->values; v; v = v->next)
        {
            if (first)
                patterns["else"] = "";
            else
                patterns["else"] = "else ";
            patterns["valuestring"] = v->name;
            output_SOURCE_enum_json2proto(json2proto_enumsrcbody, patterns);
            output_SOURCE_enum_proto2json(proto2json_enumsrcbody, patterns);
            first = false;
        }
        patterns["json2proto_enumsrcbody"] = json2proto_enumsrcbody.str();
        patterns["proto2json_enumsrcbody"] = proto2json_enumsrcbody.str();
        output_SOURCE_enum(out_cc, patterns);
    }

    for (ProtoFileMessage * m = protoFile->messages; m; m = m->next)
        emitOneMessage(patterns, out_h, out_cc, m);

    output_HEADER_bottom(out_h, patterns);
    output_SOURCE_bottom(out_cc, patterns);

    return true;
}
