//
// Created by ubuntu-dev on 24/10/18.
//

#ifndef A2SDN_FLOWTABLE_H
#define A2SDN_FLOWTABLE_H

#include <sys/types.h>

#include <vector>

struct flowEntry {
    uint srcIP_lo;
    uint srcIP_hi;
    uint destIP_lo;
    uint destIP_hi;
    uint actionType;
    uint actionVal;
    uint pri;
    uint pktCount;
};
typedef std::vector<flowEntry> flowTable;

#define DELIVER 0
#define FORWARD 1
#define DROP 2
#define MIN_PRI 4
#define MAX_IP 1000

#endif //A2SDN_FLOWTABLE_H
