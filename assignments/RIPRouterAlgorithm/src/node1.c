#include "node.h"

extern int initcost1[NODESUM];
extern int neighbour1[NODESUM];

node_class Node1;

void rtinit1(void)
{
    node_constructor(&Node1, 1);
    rtinit(&Node1, initcost1, neighbour1);
}

void rtupdate1(struct rtpkt* rcvdpkt)
{
    rtupdate(&Node1, rcvdpkt);
}

void printdt1(void)
{
    printdt(&Node1);
}

void linkhandler1(int linkid, int newcost)
/* called when cost from 1 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
{
    linkhandler(&Node1, linkid, newcost);
}