
#include <string>
#include <map>

class asm_parser
{
    bool debug;
public:
    asm_parser(bool _debug = false);
    ~asm_parser(void);
    bool parse_file(const std::string &fname);

    struct func_desc {
        unsigned int addr;
        std::string func;
    };
    struct pc_desc {
        unsigned int addr;
        std::string line;
        func_desc * func;
        pc_desc * prev;
    };

    typedef std::map<unsigned int, func_desc*> func_map_t;
    func_map_t  func_map;
    typedef std::map<unsigned int, pc_desc*> pc_map_t;
    pc_map_t pc_map;
};
