
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

void tokenizer_init(FILE *in);
extern int yylex(void);

#endif /* __TOKENIZER_H__ */
