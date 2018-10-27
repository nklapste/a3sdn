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
#include "flow.h"

#define NULL_ID "null"

using namespace std;

#define DELIVER 0
#define FORWARD 1
#define DROP 2
#define MIN_PRI 4
#define MAX_IP 1000

/**
 * Traffic file item.
 */
typedef tuple<uint, uint, uint> trafficFileItem;

class Switch {
public:
    Switch(string &switchID, string &leftSwitchID, string &rightSwitchID, string &trafficFile, uint IPLow, uint IPHigh);

    Switch(uint switchID, uint neighbors, int leftSwitchID, int rightSwitchID, uint IPLow, uint IPHigh);

    void list();

    uint getID();

    uint getIPHigh();

    uint getIPLow();

    int getLeftSwitchID();

    int getRightSwitchID();

    int getRule(FlowEntry &flowEntry, uint switchID, uint srcIP, uint dstIP);

    void start();

private:
    /**
     * ID of the switch itself
     */
    uint switchID;

    /**
     * The number of neighboring switches.
     */
    uint neighbors;
    /**
     * ID of the "left" switch to connect to
     */
    int leftSwitchID;

    /**
     * ID of the "right" switch to connect to
     */
    int rightSwitchID;

    string trafficFile;
    uint IPHigh;
    uint IPLow;
    FlowTable flowTable;
    vector<Connection> connections;

    /**
     * Counts of {@code Packets} received.
     */
    uint rOpenCount = 0;
    uint rAddCount = 0;
    uint rAckCount = 0;
    uint rRelayCount = 0;
    uint rQueryCount = 0;

    /**
     * Counts of {@code Packets} transmitted.
     */
    uint tOpenCount = 0;
    uint tAddCount = 0;
    uint tAckCount = 0;
    uint tRelayCount = 0;
    uint tQueryCount = 0;

};

#endif //A2SDN_SWITCH_H
