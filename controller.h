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
#include "gate.h"

#define MIN_SWITCHES 1
#define MAX_SWITCHES 7

#define CONTROLLER_ID 0
#define CONTROLLER_MODE "cont"

using namespace std;

class Controller : public Gate {
public:
    explicit Controller(uint nSwitches);

    void start() override;

private:
    uint nSwitches;
    vector<Switch> switches;

    FlowEntry makeFlowEntry(uint switchID, uint srcIP, uint dstIP);

    void sendACKPacket(Connection connection);

    void sendADDPacket(Connection connection, FlowEntry flowEntry);

    void respondOPENPacket(Connection connection, Message message);

    void respondQUERYPacket(Connection connection, Message message);

    void list() override;
};

#endif //A2SDN_CONTROLLER_H
