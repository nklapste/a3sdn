/**
 * switch.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_SWITCH_H
#define A2SDN_SWITCH_H

#include <chrono>
#include <ctime>
#include <string>
#include <vector>

#include "connection.h"
#include "packet.h"
#include "flow.h"
#include "gate.h"
#include "port.h"
#include "switchid.h"
#include "address.h"

using namespace std;
using namespace chrono;

/**
 * Definitions of the various ports used by the switches.
 */
#define PORT_0 0 // controller port/connection index
#define PORT_1 1 // left switch port/connection index
#define PORT_2 2 // right switch port/connection index
#define PORT_3 3 // self switch port

class Switch : public Gate {
public:
    Switch(SwitchID switchID, SwitchID leftSwitchID, SwitchID rightSwitchID, string &trafficFile, uint IPLow,
           uint IPHigh, Address address, Port port);

    Switch(SwitchID switchID, SwitchID leftSwitchID, SwitchID rightSwitchID, uint IPLow, uint IPHigh, Address address,
           Port port);

    Address getServerAddr();

    uint getIPLow() const;

    uint getIPHigh() const;

    SwitchID getSwitchID() const;

    SwitchID getLeftSwitchID() const;

    SwitchID getRightSwitchID() const;

    void start() override;

private:
    /**
     * ID of this switch. (e.g. sw1)
     *
     * Built from the {@code gateID} of this switch.
     */
    SwitchID switchID;

    string trafficFile;

    /**
     * ID of the "left" switch to connect to. (Port 1)
     */
    SwitchID leftSwitchID;

    /**
     * ID of the "right" switch to connect to. (Port 2)
     */
    SwitchID rightSwitchID;

    uint IPLow;

    uint IPHigh;

    Address address;

    FlowTable flowTable;

    vector<Connection> connections;

    vector<Packet> unsolvedPackets;

    string &parseTrafficFileLine(int socketFD, string &line);

    int getFlowEntryIndex(uint srcIP, uint dstIP);

    void sendOPENPacket(int socketFD);

    void sendQUERYPacket(int socketFD, uint srcIP, uint dstIP);

    void sendRELAYPacket(Connection connection, uint srcIP, uint dstIP, SwitchID dstSwitchID);

    void respondACKPacket();

    void respondADDPacket(Message message);

    void respondRELAYPacket(int socketFD, Message message);

    void resolveUnsolvedPackets();

    int resolvePacket(uint srcIP, uint dstIP);

    void list() override;

    void listSwitchStats();

    /* functionality for handling trafficFile delays */
    milliseconds endTime = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );

    void setDelay(milliseconds interval);

    bool delayPassed();

    void check_sock(int socketFD, char *tmpbuf) override;

    void check_connection(int connectionFD, int socketFD, Connection connection);

    void check_trafficFile(int socketFD, ifstream &trafficFileStream);
};

#endif //A2SDN_SWITCH_H
