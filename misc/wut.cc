
#include <iostream>
#include <typeinfo>
#include <string>

using namespace std;

int
main()
{
    const float * a;
    string b = typeid(a).name();
    swap(b[1], b[2]);
    cout << b << endl;
    return 0;
}
