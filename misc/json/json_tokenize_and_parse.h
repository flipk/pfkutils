
#ifdef __JSON_PARSER_INTERNAL__
#define YY_DECL int json_tokenizer_lex(YYSTYPE *yylval, yyscan_t yyscanner)
#endif /* __JSON_PARSER_INTERNAL__ */

void json_parser_debug_tokenize(FILE *f);
// this could return any Property::propertyType, check.
SimpleJson::Property * json_parser(FILE *f);
