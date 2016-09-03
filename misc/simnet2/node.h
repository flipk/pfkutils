/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "thread_slinger.h"
#include "pfkposix.h"
#include <vector>
#include <sstream>

class NodeMessage : public ThreadSlinger::thread_slinger_message
{
public:
    enum {
        DISCOVER,
        STOP
    } type;
    int discover_dest;
    std::vector<int> discover_path;
    NodeMessage(void) { }
    NodeMessage(const NodeMessage *other);
};

class Node : public pfk_pthread {
    ThreadSlinger::thread_slinger_queue<NodeMessage>   q;
    /*virtual*/ void * entry(void);
    /*virtual*/ void send_stop(void);
    std::vector<Node*> neighbors;
    int id;
    std::ostringstream prt;
    void print(void);
public:
    Node(int id);
    ~Node(void);
    void add_neighbor(Node *n) { neighbors.push_back(n); }
    void enqueue(NodeMessage *m) { q.enqueue(m); }
    const int get_id(void) const { return id; }
};
