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

/**
 * Min/Max definitions for various FlowEntry components.
 */
#define MIN_IP 0      /* srcIP_lo min */
#define MAX_IP 1000   /* srcIP_hi max */
#define MIN_PRI 4     /* pri min */

/**
 * Definitions of the various FlowEntry actionTypes.
 */
#define DELIVER 0
#define FORWARD 1
#define DROP 2

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
