#include <setjmp.h>

jmp_buf jb;

void 
sig_handler() 
{
  longjmp(jb, 1);
}
