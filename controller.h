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
#include "port.h"

#define MIN_SWITCHES 1
#define MAX_SWITCHES 7

#define CONTROLLER_MODE "cont"

using namespace std;

typedef tuple<int, char *> ClientSocketConnection;

class Controller : public Gate {
public:
    explicit Controller(uint nSwitches, Port port);

    void start() override;

private:
    uint nSwitches;

    vector<Switch> switches;

    vector<ClientSocketConnection> clientSocketConnections;

    FlowEntry makeFlowEntry(SwitchID switchID, uint srcIP, uint dstIP, SwitchID lastSwitchID);

    void sendACKPacket(int socketFD, SwitchID switchID);

    void sendADDPacket(int socketFD, FlowEntry flowEntry, SwitchID switchID);

    void respondOPENPacket(int socketFD, Message message);

    void respondQUERYPacket(int socketFD, Message message);

    void list() override;

    void listControllerStats();

    void check_sock(int socketFD, char *tmpbuf) override;
};

#endif //A2SDN_CONTROLLER_H
