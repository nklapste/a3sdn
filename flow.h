/**
 * a2sdn flow.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_FLOW_H
#define A2SDN_FLOW_H

#include <sys/types.h>
#include <tuple>
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
    uint srcIPLow;
    uint srcIPHigh;
    uint dstIPLow;
    uint dstIPHigh;
    uint actionType;
    uint actionVal;
    uint pri;
    uint pktCount;
};

/* == > and < operators defined for sorting and deduping purposes */
inline bool operator==(const FlowEntry& lhs, const FlowEntry& rhs)
{
    return tie(lhs.srcIPLow, lhs.srcIPHigh, lhs.dstIPLow, lhs.dstIPHigh, lhs.actionType, lhs.actionVal, lhs.pri) ==
           tie(rhs.srcIPLow, rhs.srcIPHigh, rhs.dstIPLow, rhs.dstIPHigh, rhs.actionType, rhs.actionVal, rhs.pri);
}

inline bool operator<(const FlowEntry& lhs, const FlowEntry& rhs)
{
    return tie(lhs.srcIPLow, lhs.srcIPHigh, lhs.dstIPLow, lhs.dstIPHigh, lhs.actionType, lhs.actionVal, lhs.pri, lhs.pktCount) <
            tie(rhs.srcIPLow, rhs.srcIPHigh, rhs.dstIPLow, rhs.dstIPHigh, rhs.actionType, rhs.actionVal, rhs.pri, rhs.pktCount);
}

inline bool operator>(const FlowEntry& lhs, const FlowEntry& rhs)
{
    return tie(lhs.srcIPLow, lhs.srcIPHigh, lhs.dstIPLow, lhs.dstIPHigh, lhs.actionType, lhs.actionVal, lhs.pri, lhs.pktCount) >
           tie(rhs.srcIPLow, rhs.srcIPHigh, rhs.dstIPLow, rhs.dstIPHigh, rhs.actionType, rhs.actionVal, rhs.pri, rhs.pktCount);
}

typedef vector<FlowEntry> FlowTable;

/**
 * Convert a {@code actionType} number into its human readable string name.
 *
 * @param actionType {@code uint}
 * @return {@code std::string} the human readable string name of the actionType.
 */
string toActionName(uint actionType){
    if (actionType == DELIVER) {
        return "DELIVER";
    } else if (actionType == FORWARD) {
        return  "FORWARD";
    } else if (actionType == DROP) {
        return  "DROP";
    } else {
        printf("WARNING: unknown actionType: %u unable to match to a human readable string name\n", actionType);
        return to_string(actionType);
    }
}

#endif //A2SDN_FLOW_H
