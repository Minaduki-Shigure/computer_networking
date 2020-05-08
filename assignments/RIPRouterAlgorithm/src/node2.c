#include "node.h"

int initcost2[4] = {3, 1, 0, 2};
int neighbour2[4] = {1, 1, 0, 1};

node_class Node2;

void rtinit2(void)
{
    node_constructor(&Node2, 2);
    rtinit(&Node2, initcost2);
}

void rtupdate2(struct rtpkt* rcvdpkt)
{
    rtupdate(&Node2, rcvdpkt);
}

void printdt2(void)
{
    printdt(&Node2);
}

void linkhandler2(int linkid, int newcost)
/* called when cost from 2 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
{
    linkhandler(&Node2, linkid, newcost);
}