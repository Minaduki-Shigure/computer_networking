#include "node.h"

extern int initcost2[NODESUM];
extern int neighbour2[NODESUM];

node_class Node2;

void rtinit2(void)
{
    node_constructor(&Node2, 2);
    rtinit(&Node2, initcost2, neighbour2);
}

void rtupdate2(struct rtpkt* rcvdpkt)
{
    rtupdate(&Node2, rcvdpkt);
}

void printdt2(void)
{
    printdt(&Node2);
}