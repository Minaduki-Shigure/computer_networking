#include "node.h"

int initcost3[NODESUM] = {7, 999, 2, 0};
int neighbour3[NODESUM] = {1, 0, 1, 0};

node_class Node3;

void rtinit3(void)
{
    node_constructor(&Node3, 3);
    rtinit(&Node3, initcost3, neighbour3);
}

void rtupdate3(struct rtpkt* rcvdpkt)
{
    rtupdate(&Node3, rcvdpkt);
}

void printdt3(void)
{
    printdt(&Node3);
}