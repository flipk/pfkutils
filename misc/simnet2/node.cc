
#include "node.h"
#include "printer.h"

#include <iostream>

using namespace std;

NodeMessage :: NodeMessage(const NodeMessage *other)
{
    type = other->type;
    if (type == DISCOVER)
    {
        discover_dest = other->discover_dest;
        discover_path = other->discover_path;
    }
}

Node :: Node(int _id)
    : id(_id)
{
    prt << "Node " << id << " initialized\n";
    print();
}

Node :: ~Node(void)
{
    stopjoin();
    prt << "Node " << id << " destroyed\n";
    print();
}

void
Node :: print(void)
{
    printer.print(prt.str());
    prt.str("");
}

void *
Node :: entry(void)
{
    bool done = false;
    int ind;
    prt << "Node " << id << " running, neighbors: ";
    for (ind = 0; ind < neighbors.size(); ind++)
        prt << neighbors[ind]->get_id() << " ";
    prt << "\n";
    print();
    while (!done)
    {
        NodeMessage * m = q.dequeue(-1);
        NodeMessage * nm;
        if (!m)
            continue;
        switch (m->type)
        {
        case NodeMessage::DISCOVER:
        {
            bool found_me = false;
            prt << "Node " << id << " got discover: ";
            for (ind = 0; ind < m->discover_path.size(); ind++)
            {
                int pid = m->discover_path[ind];
                prt << pid << " ";
                if (pid == id)
                    found_me = true;
            }
            bool found_dest = (id == m->discover_dest);
            if (found_dest)
                prt << " **** PATH FOUND **** ";
            prt << "\n";
            print();
            if (found_me == false && found_dest == false)
                for (ind = 0; ind < neighbors.size(); ind++)
                {
                    nm = new NodeMessage(m);
                    nm->discover_path.push_back(id);
                    neighbors[ind]->enqueue(nm);
                }
            break;
        }
        case NodeMessage::STOP:
            prt << "Node " << id << " got stop message\n";
            print();
            done = true;
            break;
        }
        delete m;
    }
    prt << "Node " << id << " exiting\n";
    print();
    return NULL;
}

void
Node :: send_stop(void)
{
    if (running())
    {
        prt << "Sending stop to node " << id << "\n";
        print();
        NodeMessage * m = new NodeMessage;
        m->type = NodeMessage::STOP;
        q.enqueue(m);
    }
}
