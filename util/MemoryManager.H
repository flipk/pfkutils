
void * operator new(size_t sz, const char * file,
                    int line, const char * function);
void * operator new[](size_t sz, const char * file,
                      int line, const char * function);

#define NEW new(__FILE__,__LINE__,__FUNCTION__)
