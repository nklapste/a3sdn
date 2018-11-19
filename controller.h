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
#include <poll.h>

#include "connection.h"
#include "flow.h"
#include "packet.h"
#include "switch.h"
#include "gate.h"
#include "port.h"

#define MIN_SWITCHES 1
#define MAX_SWITCHES 7

#define CONTROLLER_ID 0
#define CONTROLLER_MODE "cont"

using namespace std;

class Controller : public Gate {
public:
    explicit Controller(uint nSwitches, Port port);

    void start() override;

private:
    uint nSwitches;
    vector<Switch> switches;

    FlowEntry makeFlowEntry(uint switchID, uint srcIP, uint dstIP);

    void sendACKPacket(int socketFD);

    void sendADDPacket(int socketFD, FlowEntry flowEntry);

    void respondOPENPacket(int socketFD, Message message);

    void respondQUERYPacket(int socketFD, Message message);

    void list() override;

    void listControllerStats();

    void check_sock(int socketFD) override;
};

#endif //A2SDN_CONTROLLER_H
