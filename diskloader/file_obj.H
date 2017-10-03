
#include "config.h"
#include <stdint.h>

struct file_obj {
    char path[64];
    bool exists;
    uint32_t seed;
    uint32_t size;
    bool busy;


    file_obj(void);
    ~file_obj(void);
    void init(int ind);
    void create(void);
    void verify(void);
    void destroy(void);        
};
