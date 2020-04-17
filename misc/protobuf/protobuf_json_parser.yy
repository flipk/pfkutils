/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*

NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
NOTE 
NOTE if you edit this file, you must run
NOTE    ./flexbison.sh
NOTE to regenerate the cc and hh files.
NOTE 
NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 


REFERENCE: https://www.json.org/json-en.html

*/

%parse-param {yyscan_t yyscanner}
%define api.pure
%name-prefix "protobuf_json_parser_"

%{

#define YYDEBUG 1

#include <string>
#include <iostream>
#include <pthread.h>
#ifndef DEPENDING
#include "protobuf_json_tokenizer.h"
#endif
#include "tokenize_and_parse.h"
#include "protobuf_json_parser.hh"

using namespace std;

static void yyerror( yyscan_t yyscanner, const std::string e );

extern YY_DECL;
#undef  yylex
#define yylex(yylval) protobuf_json_tokenizer_lex(yylval, yyscanner)

static ProtoFile * protoFile;
static void set_package_type(
    std::string &out_package,
    std::string &out_name,
    bool &external_package,
    const std::string &combined);

%}

%union
{
    std::string  * word_value;
    int int_value;
    double float_value;
    ProtoFile * pf;
    ProtoFileEnum * pfenum;
    ProtoFileEnumValue * pfenumv;
    ProtoFileMessage * pfmsg;
    ProtoFileMessageField * pfmsgf;
    ProtoFileMessageField::occur_t  occur;
}

%token TOK_STRING TOK_WORD TOK_OPENBRACE TOK_CLOSEBRACE
%token TOK_SEMICOLON TOK_EQUAL TOK_INT

%token KW_SYNTAX KW_IMPORT KW_PACKAGE KW_ENUM KW_MESSAGE
%token KW_REQUIRED KW_REPEATED KW_OPTIONAL

%type <word_value>  TOK_STRING  TOK_WORD  ANYWORD
%type <int_value>  TOK_INT

%type <word_value>  KW_SYNTAX KW_IMPORT KW_PACKAGE KW_ENUM KW_MESSAGE
%type <word_value>  KW_REQUIRED KW_REPEATED KW_OPTIONAL

%type <pfenum>  ENUM_DEFINITION ENUM_ENTRIES
%type <pfenumv> ENUM_ENTRY
%type <pfmsg>   MESSAGE_DEFINITION MESSAGE_FIELDS
%type <pfmsgf>  MESSAGE_FIELD
%type <occur>      REQ_REP_OPT

%start PROTOFILE

%%

PROTOFILE
	: RULES
	;

// FLEX/BISON RULE : ALWAYS USE LEFT-RECURSION, not RIGHT
RULES
	: RULE
	| RULES RULE
	;

RULE
	: KW_SYNTAX TOK_EQUAL TOK_STRING TOK_SEMICOLON
        { /* ignored */ }
	| KW_IMPORT TOK_STRING TOK_SEMICOLON
        { protoFile->import_filenames.push_back(*$2); delete $2; }
	| KW_PACKAGE ANYWORD  TOK_SEMICOLON
        { protoFile->package = *$2; delete $2; }
	| ENUM_DEFINITION
        { protoFile->addEnum($1); }
	| MESSAGE_DEFINITION
        { protoFile->addMessage($1); }
	;

ENUM_DEFINITION
	: KW_ENUM ANYWORD TOK_OPENBRACE ENUM_ENTRIES TOK_CLOSEBRACE
        {
            $$ = $4;
            $$->name = *$2; delete $2;
        }
	;

ENUM_ENTRIES
	: ENUM_ENTRY
        {
            $$ = new ProtoFileEnum;
            $$->addValue($1);
        }
	| ENUM_ENTRIES ENUM_ENTRY
        {
            $$ = $1;
            $$->addValue($2);
        }
	;

ENUM_ENTRY
	: TOK_WORD TOK_EQUAL TOK_INT TOK_SEMICOLON
        {

            $$ = new ProtoFileEnumValue;
            $$->name = *$1; delete $1;
            $$->value = $3;
        }
	;

MESSAGE_DEFINITION
	: KW_MESSAGE ANYWORD TOK_OPENBRACE MESSAGE_FIELDS TOK_CLOSEBRACE
        {
            $$ = $4;
            $$->name = *$2; delete $2;
        }
	;

MESSAGE_FIELDS
	: MESSAGE_FIELD
        {
            $$ = new ProtoFileMessage;
            $$->addField($1);
        }
	| MESSAGE_FIELDS MESSAGE_FIELD
        {
            $$ = $1;
            $$->addField($2);
        }
	;

MESSAGE_FIELD
	: REQ_REP_OPT ANYWORD ANYWORD TOK_EQUAL TOK_INT TOK_SEMICOLON
        {
            $$ = new ProtoFileMessageField;
            $$->name = *$3; delete $3;
            set_package_type(
                $$->type_package,
                $$->type_name,
                $$->external_package,
                *$2);
            delete $2;
            $$->occur = $1;
            $$->number = $5;
        }
	;

REQ_REP_OPT
	: KW_REQUIRED
        { $$ = ProtoFileMessageField::REQUIRED; }
	| KW_OPTIONAL
        { $$ = ProtoFileMessageField::OPTIONAL; }
	| KW_REPEATED
        { $$ = ProtoFileMessageField::REPEATED; }
	;

// workaround for the fact that protobuf allows keywords in
// places that are field or message names; in otherwords,
// the "keywords" in the protobuf language are NOT reserved
// words and may appear in certain places but NOT be interpreted
// as keywords in those places. (Note the lexer uses yylval.word_value
// even for KW_* tokens.)
ANYWORD
	: TOK_WORD          { $$ = $1; }
	| KW_SYNTAX         { $$ = $1; }
	| KW_IMPORT         { $$ = $1; }
	| KW_PACKAGE        { $$ = $1; }
	| KW_ENUM           { $$ = $1; }
	| KW_MESSAGE        { $$ = $1; }
	| KW_REQUIRED       { $$ = $1; }
	| KW_REPEATED       { $$ = $1; }
	| KW_OPTIONAL       { $$ = $1; }
	;

%%

void
yyerror( yyscan_t yyscanner, const string e )
{
    fprintf(stderr, "error: %d: %s\n",
            protobuf_json_tokenizer_get_lineno(yyscanner), e.c_str());
    exit( 1 );
}

// the flex scanner might be reentrant,
// but the bison parser definitely is not.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// this has to be in this file to get access to yytname[]
void
protobuf_json_parser_debug_tokenize(const std::string &fname)
{
    FILE *f = fopen(fname.c_str(), "r");
    if (f == NULL)
        return;

    yyscan_t scanner;
    protobuf_json_tokenizer_lex_init ( &scanner );
    protobuf_json_tokenizer_restart(f, scanner);

    pthread_mutex_lock(&mutex);

    int c;
    do {
        YYSTYPE  yylval;
        c = protobuf_json_tokenizer_lex(&yylval, scanner);
        if (c < 256)
            printf("%d ", c);
        else
            printf("%c[31;1m %s %c[m", 27,
                   yytname[c-255],
                   27);
        switch (c)
        {
        case TOK_STRING:
        case TOK_WORD:
            printf("(\"%s\") ", yylval.word_value->c_str());
            delete yylval.word_value;
            break;

        case TOK_INT:
            printf("(%d) ", yylval.int_value);
            break;

        default:;
        }
    } while (c > 0);
    printf("\n");

    protobuf_json_tokenizer_lex_destroy ( scanner );
    pthread_mutex_unlock(&mutex);

    fclose(f);
}

static void set_package_type(
    std::string &out_package,
    std::string &out_name,
    bool &external_package,
    const std::string &combined)
{
    size_t  dotpos = combined.find_last_of('.');
    if (dotpos == std::string::npos)
    {
        out_package = protoFile->package;
        out_name = combined;
        external_package = false;
    }
    else
    {
        out_package = combined.substr(0,dotpos);
        out_name = combined.substr(dotpos+1);
        external_package = true;
    }
}

static ProtoFileMessage *
find_message(ProtoFileMessage * msgs, const std::string &tn)
{
    for (ProtoFileMessage * m = msgs; m; m = m->next)
        if (m->name == tn)
            return m;
    return NULL;
}

static ProtoFileEnum *
find_enum(ProtoFileEnum * enums, const std::string &en)
{
    for (ProtoFileEnum * e = enums; e; e = e->next)
        if (e->name == en)
            return e;
    return NULL;
}

static ProtoFileMessage *
find_message(ProtoFile * pf, const std::string &tn)
{
    ProtoFileMessage * ret;
    ret = find_message(pf->messages, tn);
    if (ret)
        return ret;
    for (ProtoFile * pf2 = pf->imports; pf2; pf2 = pf2->next)
    {
        ret = find_message(pf2->messages, tn);
        if (ret)
            return ret;
    }
    return ret;
}

static ProtoFileEnum *
find_enum(ProtoFile * pf, const std::string &en)
{
    ProtoFileEnum * ret;
    ret = find_enum(pf->enums, en);
    if (ret)
        return ret;
    for (ProtoFile * pf2 = pf->imports; pf2; pf2 = pf2->next)
    {
        ret = find_enum(pf2->enums, en);
        if (ret)
            return ret;
    }
    return ret;
}

ProtoFile *
protobuf_parser(const std::string &fname)
{
    FILE * f = fopen(fname.c_str(), "r");
    if (f == NULL)
        return NULL;

    yyscan_t scanner;
    ProtoFile * pf;
    pthread_mutex_lock(&mutex);
    pf = protoFile = new ProtoFile;
    protobuf_json_tokenizer_lex_init ( &scanner );
    protobuf_json_tokenizer_restart(f, scanner);
    protobuf_json_parser_parse(scanner);
    protobuf_json_tokenizer_lex_destroy ( scanner );
    pthread_mutex_unlock(&mutex);

    fclose(f);

    for (size_t ind = 0; ind < pf->import_filenames.size(); ind++)
    {
        ProtoFile * pf2 = protobuf_parser(pf->import_filenames[ind]);
        if (pf2)
            pf->addImport(pf2);
    }


    // now resolve all typetype and type_msg/type_enum
    for (ProtoFileMessage * m = pf->messages; m; m = m->next)
    {
        for (ProtoFileMessageField * mf = m->fields; mf; mf = mf->next)
        {
            const std::string &tn = mf->type_name;
            if (mf->external_package)
            {
                const std::string &pkg = mf->type_package;
                ProtoFileMessage * cm = find_message(pf,tn);
                if (cm)
                {
                    mf->typetype = ProtoFileMessageField::MSG;
                    mf->type_msg = cm;
                }
                else
                {
                    ProtoFileEnum * e = find_enum(pf,tn);
                    if (e)
                    {
                        mf->typetype = ProtoFileMessageField::ENUM;
                        mf->type_enum = e;
                    }
                }
            }
            else
            {
                // default is UNARY if we don't find it.
                mf->typetype = ProtoFileMessageField::UNARY;
                if (tn == "bool")
                {
                    mf->typetype = ProtoFileMessageField::BOOL;
                }
                else if (tn == "string" || tn == "bytes")
                {
                    mf->typetype = ProtoFileMessageField::STRING;
                }
                else
                {
                    ProtoFileMessage * cm = find_message(pf->messages,tn);
                    if (cm)
                    {
                        mf->typetype = ProtoFileMessageField::MSG;
                        mf->type_msg = cm;
                    }
                    else
                    {
                        ProtoFileEnum * e = find_enum(pf->enums,tn);
                        if (e)
                        {
                            mf->typetype = ProtoFileMessageField::ENUM;
                            mf->type_enum = e;
                        }
                    }
                }
            }
        }
    }

    return pf;
}

// ----------------------- operator<< ----------------------------------

std::ostream &operator<<(std::ostream &strm, const ProtoFile *pf)
{
    strm << "imports: \n";
    for (size_t ind = 0; ind < pf->import_filenames.size(); ind++)
        strm << "   " << pf->import_filenames[ind] << "\n";
    if (pf->imports)
        strm << pf->imports;
    strm << "package " << pf->package << "\n";
    if (pf->enums)
        strm << pf->enums;
    if (pf->messages)
        strm << pf->messages;
    return strm;
}

std::ostream &operator<<(std::ostream &strm, const ProtoFileMessage *m)
{
    strm << "message " << m->name << " {\n";
    if (m->fields)
        strm << m->fields;
    strm << "}\n";
    if (m->next)
        strm << m->next;
    return strm;
}

std::ostream &operator<<(std::ostream &strm, const ProtoFileMessageField *mf)
{
    strm << "  ";
    switch (mf->occur)
    {
    case ProtoFileMessageField::OPTIONAL: strm << "optional"; break;
    case ProtoFileMessageField::REQUIRED: strm << "required"; break;
    case ProtoFileMessageField::REPEATED: strm << "repeated"; break;
    }
    strm << " ";
    if (mf->external_package)
        strm << mf->type_package << ".";
    strm << mf->type_name
         << " " << mf->name
         << " = " << mf->number
         << " [";
    if (mf->external_package)
        strm << "EXTERNAL ";
    switch (mf->typetype)
    {
    case ProtoFileMessageField::ENUM:
        if (mf->type_enum)
        {
            strm << "ENUM ";
            if (mf->external_package)
                strm << mf->type_package << ".";
            strm << mf->type_enum->name;
        }
        break;
    case ProtoFileMessageField::MSG:
        if (mf->type_msg)
        {
            strm << "MSG ";
            if (mf->external_package)
                strm << mf->type_package << ".";
            strm << mf->type_msg->name;
        }
        break;
    case ProtoFileMessageField::STRING:
        strm << "STRING";
        break;
    case ProtoFileMessageField::BOOL:
        strm << "BOOL";
        break;
    case ProtoFileMessageField::UNARY:
        strm << "UNARY";
        break;
    }
    strm << "]\n";
    if (mf->next)
        strm << mf->next;
    return strm;
}

std::ostream &operator<<(std::ostream &strm, const ProtoFileEnum *e)
{
    strm << "enum " << e->name << " {\n";
    if (e->values)
        strm << e->values;
    strm << "}\n";
    if (e->next)
        strm << e->next;
    return strm;
}

std::ostream &operator<<(std::ostream &strm, const ProtoFileEnumValue *ev)
{
    strm << "  " << ev->name << " = " << ev->value << "\n";
    if (ev->next)
        strm << ev->next;
    return strm;
}
