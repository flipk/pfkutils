
#define TOK_LIST                                \
    TOK_ITEM(OPEN_TAG)                          \
    TOK_ITEM(CLOSE_TAG)                         \
    TOK_ITEM(CLOSE_EMPTYTAG)                    \
    TOK_ITEM(ENDTAG)                            \
    TOK_ITEM(IDENTEQ)                           \
    TOK_ITEM(IDENT)                             \
    TOK_ITEM(ATT_STRING)                        \
    TOK_ITEM(TEXT)                              \

enum {
    _dummy_item = 256,
#define TOK_ITEM(x) TOK_##x,
TOK_LIST
#undef TOK_ITEM
};

void tokenizer_init(FILE *in);
int yylex( void );
