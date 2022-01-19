#include <inttypes.h>
#include <sys/types.h>

constexpr size_t length(const char *str, size_t current_len = 0)
{
    return *str == 0 ? current_len : length(str+1, current_len+1);
}

constexpr uint32_t build_binary_literal(const char *str, size_t val = 0)
{
    return length(str) == 0 ? val : build_binary_literal(
        str+1, 2*val + ((*str == '1') ? 1 : 0));
}

constexpr uint32_t operator"" _b(const char *str)
{
    return build_binary_literal(str);
}

uint32_t some_pfk_value(void)
{
    return 10100101_b;
}

uint32_t value(void)
{
    return 0b1000;
}
