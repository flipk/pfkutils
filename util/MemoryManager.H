/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

void * operator new(size_t sz, const char * file,
                    int line, const char * function);
void * operator new[](size_t sz, const char * file,
                      int line, const char * function);

#define NEW new(__FILE__,__LINE__,__FUNCTION__)
