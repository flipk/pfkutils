
#include "msgr.h"

#include <iostream>
#include <iomanip>
#include <unistd.h>

using namespace std;

class MyClient : public PfkMsgr {
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
    }
public:
    MyClient(void) : PfkMsgr("127.0.0.1", 4000)
    {
    }
    ~MyClient(void)
    {
    }
};

int
main()
{
    MyClient  client;

    while (1)
    {
        uint8_t  buf[0x100];
        PfkMsg m;
        m.init(0x0104, buf, sizeof(buf));
        m.add_val((uint16_t) 1234);
        m.add_field(16, (void*)"this is a test");
        m.finish();
        if (client.send_msg(m))
            cerr << "s ";
        usleep(10000);
    }

    return 0;
}
