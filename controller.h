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
#include "switch.h"

#define MAX_SWITCHES 7
#define LIST_CMD "list"
#define EXIT_CMD "exit"
#define CONTROLLER_ID 0

using namespace std;

typedef tuple<uint, uint> ipRange;

class Controller {
public:
    explicit Controller(uint nSwitches);

    void list();

    void start();

    FlowEntry makeRule(uint switchId, uint srcIP, uint dstIP);

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
};

#endif //A2SDN_CONTROLLER_H
