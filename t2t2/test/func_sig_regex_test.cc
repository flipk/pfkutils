
#include "simpleRegex.h"

class TestRegex : public pxfe_regex<> {
    static const char * patt;

#define TEST_REGEX_PATTERN \
        "(^_HSMBase::Action " \
        "([_a-zA-Z][_a-zA-Z0-9]{0,30})::([_a-zA-Z][_a-zA-Z0-9]{0,30})" \
        "\\(HSMEventBase::sp_t\\)$)|" \
        "(^(.*) " \
        "([_a-zA-Z][_a-zA-Z0-9]{0,30})::([_a-zA-Z][_a-zA-Z0-9]{0,30})" \
        "\\((.*)\\)$)";

    static const int IND_CLASSNAME = 2;
    static const int IND_FUNCNAME = 3;
    static const int IND_WRONG_RETTYPE = 5;
    static const int IND_WRONG_CLASSNAME = 6;
    static const int IND_WRONG_FUNCNAME = 7;
    static const int IND_WRONG_ARGTYPE = 8;

public:
    TestRegex(void) : pxfe_regex<>(patt) { }
    void test(const char *s)
    {
        if (!exec(s))
        {
            printf("regex exec error: %s\n", err());
            return;
        }

        for (int ind = 0; ind < max_matches; ind++)
        {
            int pos, len;
            if (match(ind, &pos, &len))
                printf("match %d at pos %d len %d: '%s'\n",
                       ind, pos, len, match(s, ind).c_str());
        }

        if (match(IND_CLASSNAME))
        {
            printf("correct signature!\n"
                   "  class='%s'\n  func='%s'\n",
                   match(s, IND_CLASSNAME).c_str(),
                   match(s, IND_FUNCNAME ).c_str());
        }
        if (match(IND_WRONG_RETTYPE))
        {
            printf("incorrect signature!\n"
                   "  rettype='%s'\n  class='%s'\n"
                   "  func='%s'\n  argtype='%s'\n",
                   match(s, IND_WRONG_RETTYPE  ).c_str(),
                   match(s, IND_WRONG_CLASSNAME).c_str(),
                   match(s, IND_WRONG_FUNCNAME ).c_str(),
                   match(s, IND_WRONG_ARGTYPE  ).c_str());
        }
    }
};

const char * TestRegex :: patt = TEST_REGEX_PATTERN;

int main()
{
    TestRegex  r;

    if (!r.ok())
    {
        printf("regex compile error: %s\n", r.err());
        return 1;
    }
    else
    {
        printf("\n");
        printf("EXPECTED: correct signature\n");
        r.test("_HSMBase::Action MyHsm::top(HSMEventBase::sp_t)");
        printf("\n");

        printf("EXPECTED: incorrect signature\n");
        r.test("_HSMBase2::Action MyHsm::top(HSMEventBase::sp_t)");
        printf("\n");

        printf("EXPECTED: incorrect signature\n");
        r.test("_HSMBase::Action MyHsm::top(HSMEventBase2::sp_t)");
        printf("\n");
    }

    return 0;


}
