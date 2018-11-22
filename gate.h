/**
 * gate.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_GATE_H
#define A2SDN_GATE_H


#include <sys/param.h>
#include <vector>

#include "connection.h"
#include "port.h"

class Gate {
public:
    uint getGateID() const;

    explicit Gate(Port port);

    Port getPort() const;

    bool operator==(const Gate &g) {
        return getGateID() == g.getGateID();
    }

    bool operator>(const Gate &g) {
        return getGateID() > g.getGateID();
    }

    bool operator<(const Gate &g) {
        return getGateID() < g.getGateID();
    }

    virtual void start() {};

private:
protected:
    Port port;

    /**
     * Counts of {@code Packets} received.
     */
    uint rOpenCount = 0;
    uint rAddCount = 0;
    uint rAckCount = 0;
    uint rRelayCount = 0;
    uint rQueryCount = 0;
    uint admitCount = 0;

    /**`
     * Counts of {@code Packets} transmitted.
     */
    uint tOpenCount = 0;
    uint tAddCount = 0;
    uint tAckCount = 0;
    uint tRelayCount = 0;
    uint tQueryCount = 0;

    virtual void list() {}

    void listPacketStats();

    uint gateID;

    int getSignalFD();

    void checkSignal(int signalFD);

    void checkStdin(int stdinFD);

    string getMessage(int socketFD, char *tmpbuf);

    virtual void check_sock(int socketFD, char *tmpbuf) = 0;
};

#endif //A2SDN_GATE_H
