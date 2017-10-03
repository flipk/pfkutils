
#include <stdint.h>

struct file_obj {
    char path[64];
    bool exists;
    uint32_t seed;
    uint32_t size;
    bool busy;

    static void create_directories(int max_num_files);
    static void destroy_directories(int max_num_files);
    file_obj(void);
    ~file_obj(void);
    void init(int ind);
    void create(int max_file_size);
    void verify(void);
    void destroy(void);        

};
