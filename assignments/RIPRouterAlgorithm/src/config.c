#include "node.h"

int initcost0[NODESUM] = {0, 1, 3, 7};
int neighbour0[NODESUM] = {0, 1, 1, 1};

int initcost1[NODESUM] = {1, 0, 1, 999};
int neighbour1[NODESUM] = {1, 0, 1, 0};

int initcost2[NODESUM] = {3, 1, 0, 2};
int neighbour2[NODESUM] = {1, 1, 0, 1};

int initcost3[NODESUM] = {7, 999, 2, 0};
int neighbour3[NODESUM] = {1, 0, 1, 0};