/**
 * controller.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_CONTROLLER_H
#define A2SDN_CONTROLLER_H

#include <string>
#include <tuple>
#include <vector>

#include "connection.h"
#include "flow.h"
#include "packet.h"
#include "switch.h"

#define MIN_SWITCHES 1
#define MAX_SWITCHES 7

#define CONTROLLER_ID 0
#define CONTROLLER_MODE "cont"

using namespace std;

class Controller {
public:
    explicit Controller(uint nSwitches);

    void start();

private:
    uint nSwitches;
    vector<Switch> switches;
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

    void list();

    FlowEntry makeFlowEntry(uint switchID, uint srcIP, uint dstIP);

    void sendACKPacket(Connection connection);

    void sendADDPacket(Connection connection, FlowEntry flowEntry);

    void respondOPENPacket(Connection connection, Message message);

    void respondQUERYPacket(Connection connection, Message message);
};

#endif //A2SDN_CONTROLLER_H
