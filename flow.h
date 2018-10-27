/**
 * a2sdn flow.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */


#ifndef A2SDN_FLOW_H
#define A2SDN_FLOW_H

#include <sys/types.h>
#include <vector>

using namespace std;

struct FlowEntry {
    uint srcIP_lo;
    uint srcIP_hi;
    uint dstIP_lo;
    uint dstIP_hi;
    uint actionType;
    uint actionVal;
    uint pri;
    uint pktCount;
};

typedef vector<FlowEntry> FlowTable;

#endif //A2SDN_FLOW_H
