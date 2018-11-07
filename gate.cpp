/**
 * gate.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include "gate.h"

/**
 * Getter for a {@code Gate}'s ID.
 *
 * @return {@code unit} the ID number of the Gate.
 */
uint Gate::getGateID() const {
    return gateID;
}

/**
 * Getter for a {@code Gate}'s port number.
 *
 * @return {@code unit} the port number of the Gate.
 */
uint Gate::getPortNum() const {
    return portNum;
}

/**
 * Print statistics on the packets sent and received by the {@code Gate}.
 */
void Gate::listPacketStats() {
    printf("Packet Stats:\n");
    printf("\tReceived:    OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYIN: %u, ADMIT:%u\n",
           rOpenCount, rAckCount, rQueryCount, rAddCount, rRelayCount, admitCount);
    printf("\tTransmitted: OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYOUT:%u\n",
           tOpenCount, tAckCount, tQueryCount, tAddCount, tRelayCount);
}