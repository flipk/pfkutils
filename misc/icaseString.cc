
#include <iostream>
#include <sstream>
#include <list>
#include <map>

#include <sys/time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

class icaseString : public string
{
public:
    // the needle should be in lower case.
    size_t icaseFind(const string &needle, size_t startPos = 0) {
        size_t needleEnd = needle.size();
        if (needleEnd == 0)
            return startPos;
        size_t hayPos = startPos;
        size_t hayEnd = this->size() - needleEnd;
        for (; hayPos <= hayEnd; hayPos++)
        {
            size_t needlePos = 0;
            while (needle[needlePos] == tolower(this->at(hayPos+needlePos)))
                if (++needlePos == needleEnd)
                    return hayPos;
        }
        return npos;
    };
    icaseString(const char *init) : string(init) { }
    icaseString(const string &init) : string(init) { }
};

int
main()
{
    icaseString s = "IS THISIS A TEST ISA TEST IS";
    size_t pos = 0;

    while (1)
    {
        pos = s.icaseFind("is",pos);
        if (pos == string::npos)
            break;
        cout << "found 'is' at " << pos << endl;
        pos++;
    }

    return 0;
}
