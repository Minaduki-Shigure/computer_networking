#ifndef __NODE_H_
#define __NODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define NODESUM 4

extern struct rtpkt {
    int sourceid;       /* id of sending router sending this pkt */
    int destid;         /* id of router to which pkt being sent 
                         (must be an immediate neighbor) */
    int mincost[4];    /* min cost to node 0 ... 3 */
};

extern int TRACE;
extern int YES;
extern int NO;

extern float clocktime;

extern tolayer2(struct rtpkt packet);

struct distance_table 
{
  int costs[4][4];
};

typedef struct
{
    int id;
    struct distance_table dt;
    int mincost[4];
    int neighbour[4];
} node_class;

void node_constructor(node_class* node_ptr, int node_id);
int updatemincost(node_class* node_ptr);
void sendcost(node_class* node_ptr);
void rtinit(node_class* node_ptr, const int* initcost, const int* neighbour);
void rtupdate(node_class* node_ptr, struct rtpkt* rcvdpkt);
void printdt(node_class* node_ptr);
void linkhandler(node_class* node_ptr, int linkid, int newcost);

#endif