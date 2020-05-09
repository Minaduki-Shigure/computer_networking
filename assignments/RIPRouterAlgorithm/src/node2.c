#include "node.h"

int initcost2[4] = {3, 1, 0, 2};
int neighbour2[4] = {1, 1, 0, 1};

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