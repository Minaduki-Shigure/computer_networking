#include "node.h"

int initcost[4] = {7, 999, 2, 0};

node_class Node;

void rtinit3(void)
{
    node_constructor(&Node, 0);
    rtinit(&Node, initcost);
}

void rtupdate3(struct rtpkt *rcvdpkt)
{
    rtupdate(&Node, rcvdpkt);
}

void printdt3(void)
{
    printdt(&Node);
}

void linkhandler3(int linkid, int newcost) 
/* called when cost from 3 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
{
    linkhandler(&Node, linkid, newcost);
}