/**
 * switch.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_SWITCH_H
#define A2SDN_SWITCH_H

#include <string>
#include <vector>
#include "connection.h"

#define NULL_ID "null"

using namespace std;


#define DELIVER 0
#define FORWARD 1
#define DROP 2
#define MIN_PRI 4
#define MAX_IP 1000

struct flowEntry {
    uint srcIP_lo;
    uint srcIP_hi;
    uint dstIP_lo;
    uint dstIP_hi;
    uint actionType;
    uint actionVal;
    uint pri;
    uint pktCount;
};


/**
 * Traffic file item.
 */
typedef tuple<uint, uint, uint> trafficFileItem;

typedef std::vector<flowEntry> FlowTable;

class Switch {
public:
    Switch(string &switchId, string &leftSwitchId, string &rightSwitchId, string &trafficFile, uint ipLow, uint ipHigh);
    Switch(uint switchId, uint neighbors, uint ipLow, uint ipHigh);

    void list();
    void start();
private:
    /**
     * ID of the switch itself
     */
    uint switchId;
    uint neighbors;
    /**
     * ID of the "left" switch to connect to
     */
    uint leftSwitchId;

    /**
     * ID of the "right" switch to connect to
     */
    uint rightSwitchId;
    string trafficFile;
    uint ipHigh;
    uint ipLow;
    FlowTable flowTable;
    vector<Connection> connections;
};

#endif //A2SDN_SWITCH_H
