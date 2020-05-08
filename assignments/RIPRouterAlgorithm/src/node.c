#include "node.h"

// I was trying to make the number of the nodes adjustable
// when I realized it was fixed in the test bench.
// So, forget it.
void node_constructor(node_class* node_ptr, int node_id)
{
    node_ptr->id = node_id;
    int bias = 0;
    for (bias = 0; bias < 16; ++bias)
    {
        ((node_ptr->dt).costs) + bias = 999;
    }
    for (bias = 0; bias < 4; ++bias)
    {
        (node_ptr->mincost) + bias = 999;
    }
    printdt(node_ptr);
}

int updatemincost(node_class* node_ptr)
{
    int i, j = 0;
    int updated = 0;
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if ((node_ptr->dt).costs[i][j] < node_ptr->mincost[i])
            {
                node_ptr->mincost[i] = (node_ptr->dt).costs[i][j];
                updated = 1;
            }
        }
    }
    return updated;
}

void sendcost(node_class* node_ptr)
{
    struct rtpkt packet;
    int i = 0;
    packet.sourceid = node_ptr->id;
    memcpy(packet.mincost, node_ptr->mincost, 4 * sizeof(int));
    for (i = 0; i < 4; ++i)
    {
        if (i != node_ptr->id)
        {
            packet.destid = i;
            tolayer2(packet);
        }
    }
}

void rtinit(node_class* node_ptr, const int* initcost)
{
    int i = 0;
    for (i = 0; i < 4; ++i)
    {
        (node_ptr->dt).costs[i][i] = initcost[i];
    }
    updatemincost(node_ptr);
    sendcost(node_ptr);
}


void rtupdate(node_class* node_ptr, struct rtpkt* rcvdpkt)
{
    int i = 0;
    int sid = 0;

    if (rcvdpkt->destid != node_ptr->id)
    {
        return;
    }
    updatemincost(node_ptr);    // Just in case.

    sid = rcvdpkt->sourceid;
    for (i = 0; i < 4; ++i)
    {
        (node_ptr->dt).costs[i][sid] = rcvdpkt->mincost[i] + (node_ptr->dt).costs[sid][sid];
    }

    if (updatemincost(node_ptr))
    {
        sendcost(node_ptr);
    }
}


void printdt(node_class* node_ptr)
{
    printf("        dt for node %d     \n", node_ptr->id);
    printf("                via     \n");
    printf("   D0 |    0    1     2     3 \n");
    printf("  ----|-----------------------\n");
    printf("     0|  %3d  %3d   %3d   %3d\n", (node_ptr->dt).costs[0][0], (node_ptr->dt).costs[0][1], (node_ptr->dt).costs[0][2], (node_ptr->dt).costs[0][3]);
    printf("     1|  %3d  %3d   %3d   %3d\n", (node_ptr->dt).costs[1][0], (node_ptr->dt).costs[1][1], (node_ptr->dt).costs[1][2], (node_ptr->dt).costs[1][3]);
    printf("dest 2|  %3d  %3d   %3d   %3d\n", (node_ptr->dt).costs[2][0], (node_ptr->dt).costs[2][1], (node_ptr->dt).costs[2][2], (node_ptr->dt).costs[2][3]);
    printf("     3|  %3d  %3d   %3d   %3d\n", (node_ptr->dt).costs[3][0], (node_ptr->dt).costs[3][1], (node_ptr->dt).costs[3][2], (node_ptr->dt).costs[3][3]);
}

void linkhandler(node_class* node_ptr, int linkid, int newcost)
{

}