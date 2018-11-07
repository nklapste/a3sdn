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

class Gate {
public:
    uint getGateID() const;

    uint getPortNum() const;

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
    uint portNum;
protected:
    vector<Connection> connections;

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
};


#endif //A2SDN_GATE_H
