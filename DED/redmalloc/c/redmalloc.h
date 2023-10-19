
#ifndef __REDMALLOC_H__
#define __REDMALLOC_H__

#include <stdint.h>

void   redmallocinit(void);
void * redmalloc(size_t size, char * filename, uint32_t line_number);
void   redfree(void * ptr);
void   redcheck(int printall);

#define REDMALLOC(size) redmalloc((size), __FILE__, __LINE__)

#endif /* __REDMALLOC_H__ */
