
#include "msgr.h"

#include <iostream>
#include <iomanip>
#include <unistd.h>

using namespace std;

class MyServer : public PfkMsgr {
    /*virtual*/ void handle_connected(void)
    {
        cout << "connected\n";
    }
    /*virtual*/ void handle_disconnected(void)
    {
        cout << "disconnected\n";
    }
    /*virtual*/ void handle_msg(const PfkMsg &msg)
    {
        if (0)
        {
            cout << "got msg:\n";
            for (uint32_t fld = 0; fld < msg.get_num_fields(); fld++)
            {
                uint16_t fldlen;
                uint8_t * f = (uint8_t *) msg.get_field(&fldlen, fld);
                if (f == NULL)
                    cout << "can't fetch field " << fld << endl;
                else {
                    cout << "field " << fld << ": ";
                    for (uint32_t fldpos = 0; fldpos < fldlen; fldpos++)
                        cout << hex << setw(2) << setfill('0')
                             << (int) f[fldpos] << " ";
                    cout << endl;
                }
            }
        }
        else
            cerr << "r ";
        if (send_msg(msg))
            cerr << "s ";
    }
public:
    MyServer(void) : PfkMsgr(4000)
    {
    }
    ~MyServer(void)
    {
    }
};

int
main()
{
    MyServer  svr;

    while (1)
        sleep(1);

    return 0;
}
