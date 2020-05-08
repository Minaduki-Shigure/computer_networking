#include "node.h"

int initcost0[4] = {0, 1, 3, 7};
int neighbour0[4] = {0, 1, 1, 1};

node_class Node0;

void rtinit0(void)
{
    node_constructor(&Node0, 0);
    rtinit(&Node0, initcost0, neighbour0);
}

void rtupdate0(struct rtpkt* rcvdpkt)
{
    rtupdate(&Node0, rcvdpkt);
}

void printdt0(void)
{
    printdt(&Node0);
}

void linkhandler0(int linkid, int newcost)
/* called when cost from 0 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
{
    linkhandler(&Node0, linkid, newcost);
}