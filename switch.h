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
#include "packet.h"
#include "flow.h"

#define NULL_SWITCH_FLAG "null"
#define NULL_SWITCH_ID -1

using namespace std;

#define DELIVER 0
#define FORWARD 1
#define DROP 2

#define MIN_IP 0
#define MAX_IP 1000

#define MIN_PRI 4

#define PORT_0 0 // controller port/connection index
#define PORT_1 1 // left switch port/connection index
#define PORT_2 2 // right switch port/connection index
#define PORT_3 3 // self switch port

typedef tuple<uint, uint> IPRange;

/**
 * Traffic file item.
 */
typedef tuple<uint, uint, uint> trafficFileItem;

class Switch {
public:
    Switch(string &switchID, string &leftSwitchID, string &rightSwitchID, string &trafficFile, string &IPRangeStr);

    Switch(uint switchID, int leftSwitchID, int rightSwitchID, uint IPLow, uint IPHigh);

    uint getID();

    uint getIPLow();

    uint getIPHigh();

    int getLeftSwitchID();

    int getRightSwitchID();

    void start();

private:
    /**
     * ID of the switch itself. (Port 3)
     */
    uint switchID;

    /**
     * ID of the "left" switch to connect to. (Port 1)
     */
    int leftSwitchID;

    /**
     * ID of the "right" switch to connect to. (Port 2)
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
    uint admitCount = 0; /* special for Switch indicates number of DELIVERed packets */

    /**
     * Counts of {@code Packets} transmitted.
     */
    uint tOpenCount = 0;
    uint tAddCount = 0;
    uint tAckCount = 0;
    uint tRelayCount = 0;
    uint tQueryCount = 0;

    IPRange parseIPRange(const string &IPRange);

    uint parseSwitchID(const string &switchID);

    trafficFileItem parseTrafficFileItem(string &trafficFileLine);

    string &parseTrafficFileLine(string &line);

    void list();

    int getFlowEntryIndex(uint srcIP, uint dstIP);

    void sendOPENPacket(Connection connection);

    void sendQUERYPacket(Connection connection, uint srcIP, uint dstIP);

    void sendRELAYPacket(Connection connection, uint srcIP, uint dstIP);

    void respondACKPacket();

    void respondADDPacket(Message message);

    void respondRELAYPacket(Message message);
};

#endif //A2SDN_SWITCH_H
