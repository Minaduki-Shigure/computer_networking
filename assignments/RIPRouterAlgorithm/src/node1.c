#include "node.h"

int initcost[4] = {1, 0, 1, 999};

node_class Node;

void rtinit1(void) 
{
    node_constructor(&Node, 1);
    rtinit(&Node, initcost);
}

void rtupdate1(struct rtpkt* rcvdpkt)  
{
    rtupdate(&Node, rcvdpkt);
}

void printdt1(void)
{
    printdt(&Node);
}

void linkhandler1(int linkid, int newcost)    
/* called when cost from 1 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */	
{
    linkhandler(&Node, linkid, newcost);
}