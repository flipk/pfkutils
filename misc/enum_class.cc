#if 0
set -e -x
g++ -Wall -Werror -D__TEST_ENUM_CLASS_MAIN__ enum_class.cc -o enum_class
./enum_class
rm -f enum_class
exit 0
#endif


#include <inttypes.h>
#include <sstream>

#define STATUS_FIELDS_LIST        \
    STATUS_FIELDS_ITEM(0, ONE  )  \
    STATUS_FIELDS_ITEM(1, TWO  )  \
    STATUS_FIELDS_ITEM(2, THREE)  \
    STATUS_FIELDS_ITEM(3, FOUR )  \
    STATUS_FIELDS_ITEM(4, FIVE )


typedef uint32_t status_field_mask_type;
//typedef uint64_t status_field_mask_type;
enum class status_field_mask : status_field_mask_type {
    NONE  = 0,
#define STATUS_FIELDS_ITEM(val, name) name = (1 << val),
    STATUS_FIELDS_LIST
#undef  STATUS_FIELDS_ITEM
#define STATUS_FIELDS_ITEM(val, name) | (1 << val)
    ALL   =  (0 STATUS_FIELDS_LIST)
#undef  STATUS_FIELDS_ITEM
};
static inline status_field_mask operator|(const status_field_mask a,
                                          const status_field_mask b)
{ return (status_field_mask)
        ((status_field_mask_type) a | (status_field_mask_type) b); }
static inline bool operator&(const status_field_mask a,
                             const status_field_mask b)
{ return ((status_field_mask_type) a & (status_field_mask_type) b) != 0; }
static inline void operator|=(status_field_mask &a,
                              const status_field_mask b)
{ a = a | b; }
static std::ostream& operator<<(std::ostream& os,
                                const status_field_mask m) {
    if (m == status_field_mask::NONE)
        os << "NONE";
    else if (m == status_field_mask::ALL)  // optional
        os << "ALL";
    else {
        bool first = true;
#define STATUS_FIELDS_ITEM(val, name)           \
        if (m & status_field_mask::name) {      \
            if (!first) os << "|";            \
            os << #name;                        \
            first = false;                      \
        }
        STATUS_FIELDS_LIST
#undef  STATUS_FIELDS_ITEM
    }
    return os;
}
static inline std::string status_field_str(status_field_mask m)
{ std::ostringstream  ostr; ostr << m; return ostr.str(); }




#ifdef __TEST_ENUM_CLASS_MAIN__

#include <iostream>

int main()
{
    status_field_mask a = status_field_mask::NONE;
    status_field_mask b = status_field_mask::ONE;
    status_field_mask c = status_field_mask::ALL;

    b |= status_field_mask::FOUR;

    std::cout << "a = " << a << ", "
              << "b = " << b << ", "
              << "c = " << c << "\n";

    return 0;
}

#endif // __TEST_ENUM_CLASS_MAIN__
