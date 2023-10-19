
#include <stdio.h>
#include <string>
#include "tokenizer.h"
#include TOKENIZER_LL_HDR

const char * token_names[] = {
    "__dummy",
#define TOK_ITEM(x)  #x ,
TOK_LIST
#undef  TOK_ITEM
};

int
main()
{
    std::string xml(
        "<user   g\nu=\"one\"\tv=\"two\">\n"
        "  <userid type=\"pod\">1</userid>\n"
        "  <firstname type=\"text\">fir1</firstname>\n"
        "  <lastname type=\"text\">las1</lastname>\n"
        "  <mi type=\"text\"/>\n"
        "  <SSN type=\"pod\">11</SSN>\n"
        "  <balance type=\"pod\">4</balance>\n"
        "  <proto type=\"blob\">746869732069732070726f746f20626c6f62</proto>\n"
        "  <test2 type=\"bool\">false</test2>\n"
        "  <test3 type=\"sample::library2::EnumField_t\">ENUM_TWO</test3>\n"
        "  <checkouts index=\"0\" type=\"subtable\">\n"
        "    <bookid2 type=\"pod\">2</bookid2>\n"
        "    <userid2 type=\"pod\">1</userid2>\n"
        "    <duedate type=\"pod\">5</duedate>\n"
        "  </checkouts>\n"
        "  <checkouts index=\"1\" type=\"subtable\">\n"
        "    <bookid2 type=\"pod\">3</bookid2>\n"
        "    <userid2 type=\"pod\">1</userid2>\n"
        "    <duedate type=\"pod\">6</duedate>\n"
        "  </checkouts>\n"
        "</user>\n"
        );


    FILE *f = fmemopen((void*) xml.c_str(), xml.size(), "r");

    yyscan_t scanner;

    xml_tokenizer_lex_init ( &scanner );
    xml_tokenizer_restart(f, scanner);

    int c;
    do {
        c = xml_tokenizer_lex(scanner);
        if (c < 256)
            printf("%c", c);
        else
            printf("%c[31;1m %s %c[m", 27, token_names[c-256], 27);
    } while (c > 0);
    printf("\n");

    xml_tokenizer_lex_destroy ( scanner );

    return 0;
}
