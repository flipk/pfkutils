
#include <string>
#include <vector>
#include <iostream>

using namespace std;

typedef vector<string> stringVector;

void printVec(const string &input, const vector<string> &res)
{
    cout << "for input '" << input << "' the vector contents are:" << endl;
    for (int ind = 0; ind < res.size(); ind++)
    {
        const string &s = res[ind];
        cout << ind << ": '" << s << "'" << endl;
    }
}

vector<string> splitString( const string &line )
{
    vector<string> ret;
    size_t pos;
    for (pos = 0; ;)
    {
        size_t found = line.find_first_of("\t ",pos);
        if (found == string::npos)
        {
            if (pos != line.size())
                ret.push_back(line.substr(pos,line.size()-pos));
            break;
        }
        if (found > pos)
            ret.push_back(line.substr(pos,found-pos));
        pos = found+1;
    }
    printVec(line, ret);
    return ret;
}


int
main()
{
    splitString("one");
    splitString("one two");
    splitString("one\ttwo");
    splitString("one\t     \t   \t  two");
    splitString("\t \t \t \t one\t     \t   \t  two\t \t \t \t ");
    splitString(" one two");
    splitString(" one two ");
    splitString("   one   two  three\tfour five\tsix seven   ");
    return 0;
}
