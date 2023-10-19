
#include "printer.h"

#include <iostream>

using namespace std;

//static
Printer * Printer::_instance = NULL;

Printer :: Printer(void)
{
    _instance = this;
}

Printer :: ~Printer(void)
{
    _instance = NULL;
    stopjoin();
}

void *
Printer :: entry(void)
{
    bool done = false;
    cout << "Printer thread started\n";
    while (!done)
    {
        PrintMessage * m = q.dequeue(-1);
        if (m == NULL)
            continue;
        if (m->msg == "__SEND_STOP__")
            done = true;
        else
            cout << m->msg;
        delete m;
    }
    cout << "Printer thread done\n";
    return NULL;
}

void
Printer :: send_stop(void)
{
    PrintMessage * m = new PrintMessage("__SEND_STOP__");
    q.enqueue(m);
}

Printer printer;
