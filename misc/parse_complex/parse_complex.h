
#include <regex>
#include <complex>
#include <string>
#include <iostream>

class ComplexConverter {
    std::regex   reg;
    std::smatch  matches;
    template <typename T>
    static T stringToNumber(const std::string &s)
    {
        if constexpr (std::is_same_v<T, float>) {
            return std::stof(s);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(s);
        } else if constexpr (std::is_same_v<T, long double>) {
            return std::stold(s);
        } else {
            throw std::runtime_error("Unsupported type for stringToNumber");
        }
    }
    template <typename T>
    bool cvt_number(const std::string &s,  T &value,
                    int mantissa_ind, int exponent_ind)
    {
        if (matches[mantissa_ind].matched)
        {
            T v = stringToNumber<T>(matches[mantissa_ind].str());
            if (matches[exponent_ind].matched)
            {
                T exp = stringToNumber<T>(matches[exponent_ind].str());
                v *= std::pow(10.0, exp);
            }
            value = v;
            return true;
        }
        return false;
    }
public:
    enum groups {
        REAL_MANTISSA  = 1,
        REAL_EXPONENT  = 2,
        IMAG1_MANTISSA = 3,
        IMAG1_EXPONENT = 4,
        IMAG2_MANTISSA = 5,
        IMAG2_EXPONENT = 6
    };
    ComplexConverter(void) : reg(
        R"regex(^(?:(?:(?:(?:((?:[+-]?)(?:(?:\d+(?:\.\d*)?|\d*\.\d+))))(?:[eE]([+-]?\d+))?)?(?:((?:[+-])(?:(?:\d+(?:\.\d*)?|\d*\.\d+)))(?:[eE]([+-]?\d+))?[ij])?)|(?:(?:((?:[+-]?)(?:(?:\d+(?:\.\d*)?|\d*\.\d+)))(?:(?:[eE]([+-]?\d+)))?[ij])))$)regex"
        ) { }
    ~ComplexConverter(void) { }
    template <typename T>
    bool convert(const std::string &s, std::complex<T> &result)
    {
        if (!std::regex_search(s, matches, reg))
            return false;
        T v;
        result.real(0.0);
        result.imag(0.0);
        if (cvt_number(s, v, REAL_MANTISSA, REAL_EXPONENT))
            result.real(v);
        if (cvt_number(s, v, IMAG1_MANTISSA, IMAG1_EXPONENT))
            result.imag(v);
        else if (cvt_number(s, v, IMAG2_MANTISSA, IMAG2_EXPONENT))
            result.imag(v);
        return true;
    }
};
