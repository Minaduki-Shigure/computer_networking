#include "node.h"

int initcost3[4] = {7, 999, 2, 0};
int neighbour3[4] = {1, 0, 1, 0};

node_class Node3;

void rtinit3(void)
{
    node_constructor(&Node3, 0);
    rtinit(&Node3, initcost3);
}

void rtupdate3(struct rtpkt *rcvdpkt)
{
    rtupdate(&Node3, rcvdpkt);
}

void printdt3(void)
{
    printdt(&Node3);
}

void linkhandler3(int linkid, int newcost) 
/* called when cost from 3 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
{
    linkhandler(&Node3, linkid, newcost);
}