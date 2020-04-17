
#define YY_DECL int json_tokenizer_lex(YYSTYPE *yylval, yyscan_t yyscanner)

void json_parser_debug_tokenize(FILE *f);
SimpleJson::ObjectProperty * json_parser(FILE *f);
