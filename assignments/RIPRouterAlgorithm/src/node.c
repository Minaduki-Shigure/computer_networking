#include "node.h"

// I was trying to make the number of the nodes adjustable
// when I realized it was fixed in the test bench.
// So, forget it.
void node_constructor(node_class* node_ptr, int node_id)
{
    node_ptr->id = node_id;
    int* ptr;
    int bias = 0;
    
    ptr = (node_ptr->dt).costs;
    for (bias = 0; bias < 16; ++bias)
    {
        ptr[bias] = 999;   // Anyway, it's a bias.
    }
    for (bias = 0; bias < 4; ++bias)
    {
        node_ptr->mincost[bias] = 999;
    }
}

int updatemincost(node_class* node_ptr)
{
    int i, j = 0;
    int updated = 0;
    int newmincost[4] = {999, 999, 999, 999};

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if ((node_ptr->dt).costs[i][j] < newmincost[i])
            {
                newmincost[i] = (node_ptr->dt).costs[i][j];
            }
        }
        if (node_ptr->mincost[i] != newmincost[i])
        {
            updated = 1;
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
        if ((node_ptr->neighbour)[i])
        {
            packet.destid = i;
            tolayer2(packet);
            printf("\033[31mTime = %.3f. Node %d sent packet to Node %d : %d %d %d %d\033[0m\n", clocktime, node_ptr->id, i, packet.mincost[0], packet.mincost[1], packet.mincost[2], packet.mincost[3]);
        }
    }
}

void rtinit(node_class* node_ptr, const int* initcost, const int* neighbour)
{
    int i = 0;

    printf("\033[31mTime = %.3f. rtinit%d() has been called.\033[0m\n", clocktime, node_ptr->id);

    for (i = 0; i < 4; ++i)
    {
        (node_ptr->dt).costs[i][i] = initcost[i];
        (node_ptr->neighbour)[i] = neighbour[i];
    }
    updatemincost(node_ptr);
    sendcost(node_ptr);
}


void rtupdate(node_class* node_ptr, struct rtpkt* rcvdpkt)
{
    int i = 0;
    int sid = 0;
    int modified = 0;

    printf("\033[31mTime = %.3f. rtupdate%d() has been called.\033[0m\n", clocktime, node_ptr->id);

    if (rcvdpkt->destid != node_ptr->id)
    {
        return;
    }

    printf("\033[31mTime = %.3f. Node %d received packet from Node %d.\033[0m\n", clocktime, node_ptr->id, rcvdpkt->sourceid);

    updatemincost(node_ptr);    // Just in case.

    sid = rcvdpkt->sourceid;
    for (i = 0; i < 4; ++i)
    {
        if ((node_ptr->dt).costs[i][sid] != rcvdpkt->mincost[i] + (node_ptr->dt).costs[sid][sid])
        {
            modified = 1;
            (node_ptr->dt).costs[i][sid] = rcvdpkt->mincost[i] + (node_ptr->dt).costs[sid][sid];
        }
    }

    if (modified)
    {
        printf("\033[31mTime = %.3f. Distance Table for Node %d has been modified!\033[0m\n", clocktime, node_ptr->id);
        printdt(node_ptr);
    }

    if (updatemincost(node_ptr))
    {
        sendcost(node_ptr);
    }
}


void printdt(node_class* node_ptr)
{
    int i, j = 0;

    printf("  dest|     via     \n");

    printf("   D%d |", node_ptr->id);

    for (j = 0; j < 4; ++j)
    {
        if ((node_ptr->neighbour)[j])
        {
            printf("    %d", j);
        }
    }
    printf(" \n");

    printf("  ----|");
    for (j = 0; j < 4; ++j)
    {
        if ((node_ptr->neighbour)[j])
        {
            printf("-----", j);
        }
    }
    printf("\n");

    for (i = 0; i < 4; ++i)
    {
        if (i != node_ptr->id)
        {
            printf("     %d|", i);
            for (j = 0; j < 4; ++j)
            {
                if ((node_ptr->neighbour)[j])
                {
                    printf("  %3d", (node_ptr->dt).costs[i][j]);
                }
            }
            printf("\n");
        }
    }
}

void linkhandler(node_class* node_ptr, int linkid, int newcost)
{
    printf("\033[31mTime = %.3f. linkhandler%d() has been called.\033[0m\n", clocktime, node_ptr->id);    

    if (!(node_ptr->neighbour)[linkid])
    {
        return; // The link do not exist. There must be some mistakes. Ignore.
    }

    printf("\033[31mTime = %.3f. Node %d detected the new cost of link to Node %d is now %d.\033[0m\n", clocktime, node_ptr->id, linkid, newcost);

    int i, j = 0;
    int oldcost = (node_ptr->dt).costs[linkid][linkid];

    updatemincost(node_ptr);    // Just in case.   
    for (i = 0; i < 4; ++i)
    {
        (node_ptr->dt).costs[i][linkid] = (node_ptr->dt).costs[i][linkid] - oldcost + newcost;
    }
    printf("\033[31mTime = %.3f. Distance Table for Node %d has been modified!\033[0m\n", clocktime, node_ptr->id);
    printdt(node_ptr);

    if (updatemincost(node_ptr))
    {
        sendcost(node_ptr);
    }
}