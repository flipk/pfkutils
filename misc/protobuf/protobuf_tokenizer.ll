/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*

NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
NOTE 
NOTE if you edit this file, you must run
NOTE    ./flexbison.sh
NOTE to regenerate the cc and h files.
NOTE 
NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 

*/

%option noyywrap
%option yylineno
%option reentrant
%option prefix="protobuf_tokenizer_"
%option outfile="protobuf_tokenizer.cc"
%option header-file="obj/protobuf_tokenizer.h"

%{

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <string>
#define __SIMPLE_PROTOBUF_INTERNAL__ 1
#include "protobuf_tokenize_and_parse.h"
#include PARSER_YY_HDR

using namespace std;

static string  collect_string;
#ifdef TOKENIZE_COMMENTS
static void denewline(std::string &str);
#endif

%}

ws              [\t \r\n]+
%s IN_STRING IN_COMMENT

%%

<IN_STRING>\\.   {
    // we un-escape double quotes here but
    // we leave all the other escapes alone.
    // (FYI this is un-done in operator<<(StringProperty)).
    if (yytext[1] != '"')
        collect_string += yytext[0];
    collect_string += yytext[1];
}
<IN_STRING>\"    {
    BEGIN(INITIAL);
    yylval->word_value = new string(collect_string);
    return TOK_STRING;
}
<IN_STRING>.     { collect_string += yytext[0]; }



<INITIAL>\"      { BEGIN(IN_STRING); collect_string =""; }

   /* workaround for the fact that protobuf allows keywords in
   // places that are field or message names; in otherworse,
   // the "keywords" in the protobuf language are NOT reserved
   // words and may appear in certain places but NOT be interpreted
   // as keywords in those places. (Note the parser assumes
   // yylval.word_value is populated even for KW_* tokens.)   */

<INITIAL>syntax    {
    yylval->word_value = new std::string("syntax");   return KW_SYNTAX;   };
<INITIAL>import    {
    yylval->word_value = new std::string("import");   return KW_IMPORT;   };
<INITIAL>package    {
    yylval->word_value = new std::string("package");  return KW_PACKAGE;  };
<INITIAL>enum    {
    yylval->word_value = new std::string("enum");     return KW_ENUM;     };
<INITIAL>message    {
    yylval->word_value = new std::string("message");  return KW_MESSAGE;  };
<INITIAL>required    {
    yylval->word_value = new std::string("required"); return KW_REQUIRED; };
<INITIAL>repeated    {
    yylval->word_value = new std::string("repeated"); return KW_REPEATED; };
<INITIAL>optional    {
    yylval->word_value = new std::string("optional"); return KW_OPTIONAL; };
<INITIAL>option      {
    yylval->word_value = new std::string("option");   return KW_OPTION;   };

<INITIAL>0x[0-9a-fA-F]+         {
    std::string s(yytext, yyleng);
    char * endptr = NULL;
    long val = strtol(s.c_str(), &endptr, /*base*/0);
    yylval->int_value = (int) val;
    return TOK_INT;
}

<INITIAL>[a-zA-Z][a-zA-Z0-9_\.]* {
    yylval->word_value = new std::string(yytext, yyleng);
    return TOK_WORD;
}

<INITIAL>[+-]?[0-9]+         {
    std::string s(yytext, yyleng);
    char * endptr = NULL;
    long val = strtol(s.c_str(), &endptr, /*base*/0);
    yylval->int_value = (int) val;
    return TOK_INT;
}

<INITIAL>\{               { return TOK_OPENBRACE; }
<INITIAL>\}               { return TOK_CLOSEBRACE; }
<INITIAL>;                { return TOK_SEMICOLON; }
<INITIAL>=                { return TOK_EQUAL; }
<INITIAL>{ws}             {  }

   /* use "manual input" method to find end of comment
      https://stackoverflow.com/questions/19488772/non-greedy-regular-expression-matching-in-flex
   */

<INITIAL>\/\*             {
    collect_string.assign(yytext, yyleng);
    int last_c = 0;
    while (1)
    {
        int c = yyinput(yyscanner);
        if (c == EOF)
            break;
        collect_string += (char) c;
        if (last_c == '*' && c == '/')
        {
#ifdef TOKENIZE_COMMENTS // discard comments; this was useful for testing.
            denewline(collect_string);
            yylval->word_value = new string(collect_string);
            return TOK_COMMENT;
#else
            break;
#endif
        }
        last_c = c;
    }
}

<INITIAL>\/\/.*           {
#ifdef TOKENIZE_COMMENTS // discard comments; this was useful for testing.
    collect_string.assign(yytext,yyleng);
    denewline(collect_string);
    yylval->word_value = new std::string(collect_string);
    return TOK_COMMENT;
#endif
}

     /* <INITIAL>[\r\n.]                { return yytext[0]; } */

%%

#ifdef TOKENIZE_COMMENTS
static void
denewline(std::string &str)
{
    for (size_t ind = 0; ind < str.size();)
    {
        if (str[ind] == '\r' || str[ind] == '\n')
            str.erase(ind, 1);
        else
            ind++;
    }
}
#endif

#if 0 // useful for testing lexer standalone
const char *token_names[] = {
#define TOKEN_ITEM(x)   #x ,
    TOKEN_LIST
#undef TOKEN_ITEM
};


void
protobuf_parser_debug_tokenize(FILE *f)
{
    yyscan_t scanner;
    protobuf_tokenizer_lex_init ( &scanner );
    protobuf_tokenizer_restart(f, scanner);

    int c;
    do {
        YYSTYPE  yylval;
        c = protobuf_tokenizer_lex(&yylval, scanner);
        if (c < 255)
            printf("%d ", c);
        else
            printf("%c[31;1m %s %c[m", 27, token_names[c-255], 27);
        switch (c)
        {
        case TOK_COMMENT:
        case TOK_STRING:
        case TOK_WORD:
            printf("(%s) ", yylval.word_value->c_str());
            delete yylval.word_value;
            break;

        case TOK_INT:
            printf("(%d) ", yylval.int_value);
            break;

        default:;
        }
    } while (c > 0);
    printf("\n");
}
#endif

// this exists only to remove a "defined but not used" warning
void __unused_function_protobuf_tokenizer(int c, char*d, yyscan_t s)
{ yyunput(c,d,s); }
