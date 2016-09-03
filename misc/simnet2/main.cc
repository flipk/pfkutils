
#include "node.h"
#include "printer.h"

#include <vector>

using namespace std;

#define NUM_NODES 18

int neighbors[NUM_NODES][NUM_NODES] = {

    // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17
    {  2,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  0,  3,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  0,  1,  2,  4,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  4,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  3,  5,  7,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  6,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  6,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  2,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    {  1, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 10, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 12, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 13, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 14, 15, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },

};




int
main()
{
    vector<Node*>   nodes;
    int ind_x, ind_y;
    printer.create();
    for (ind_x = 0; ind_x < NUM_NODES; ind_x++)
        nodes.push_back(new Node(ind_x));
    for (ind_x = 0; ind_x < NUM_NODES; ind_x++)
        for (ind_y = 0; ind_y < NUM_NODES; ind_y++)
        {
            int y = neighbors[ind_x][ind_y];
            if (y != -1)
                nodes[ind_x]->add_neighbor(nodes[y]);
        }
    for (ind_x = 0; ind_x < NUM_NODES; ind_x++)
        nodes[ind_x]->create();
#if 1
    sleep(1);
    NodeMessage * m = new NodeMessage;
    m->type = NodeMessage::DISCOVER;
    m->discover_dest = 17;
    nodes[7]->enqueue(m);
#endif
    sleep(1);
    for (ind_x = 0; ind_x < NUM_NODES; ind_x++)
        nodes[ind_x]->stopjoin();
    for (ind_x = 0; ind_x < NUM_NODES; ind_x++)
        delete nodes[ind_x];
    nodes.clear();
    printer.stopjoin();
    return 0;
}
