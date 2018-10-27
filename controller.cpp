/**
 * a2sdn controller.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <sys/types.h>
#include <tuple>
#include <iostream>
#include <fstream>

#include "controller.h"
#include "packet.h"

/*FIFO stuff*/
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <poll.h>
#include <algorithm>

using namespace std;

/**
 * Initialize a Controller.
 *
 * @param nSwitches
 */
Controller::Controller(uint nSwitches) : nSwitches(nSwitches) {
    if (nSwitches > MAX_SWITCHES) {
        printf("ERROR: too many switches for controller: %u\n"
               "\tMAX_SWITCHES=%u\n", nSwitches, 7);
        exit(1);
    }
    printf("Creating controller: nSwitches: %u\n", nSwitches);
    // init all potential switch connections for the controller
    for (uint switch_i = 1; switch_i <= nSwitches; ++switch_i) {
        connections.emplace_back(CONTROLLER_ID, switch_i);
    }
}

/**
 * Print the switches connected to the controller and the statistics of packets sent and received.
 */
void Controller::list() {
    printf("Switch information:\n");
    for (auto &sw: switches) {
        printf("[sw%u] port1= %i, port= %i, port3= %u-%u\n",
               sw.getId(), sw.getLeftSwitchId(), sw.getRightSwitchId(), sw.getIpLow(), sw.getIpHigh());
    }

    printf("Packet Stats:\n");
    printf("\tReceived:    OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYIN: %u\n",
           rOpenCount, rAckCount, rQueryCount, rAddCount, rRelayCount);
    printf("\tTransmitted: OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYOUT:%u\n",
           tOpenCount, tAckCount, tQueryCount, tAddCount, tRelayCount);
}

/**
 * Start the {@code Controller} loop.
 */
void Controller::start() {
    struct pollfd pfds[2 * connections.size() + 1];
    char buf[1024];

    // setup file descriptions or stdin and all connection FIFOs
    pfds[0].fd = STDIN_FILENO;
    for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
        printf("pfds[%lu] has connection: %s\n", i, connections[i - 1].getReceiveFIFOName().c_str());
        pfds[i].fd = connections[i - 1].openReceiveFIFO();
    }

    for (;;) {
        /*
         * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        poll(pfds, 2 * connections.size() + 1, 0);
        // TODO: error handling
        if (pfds[0].revents & POLLIN) {
            int r = read(pfds[0].fd, buf, 1024);
            if (!r) {
                printf("WARNING: stdin closed\n");
            }
            string cmd = string(buf);
            // trim off all whitespace
            while (!cmd.empty() && !std::isalpha(cmd.back())) cmd.pop_back();

            if (cmd == LIST_CMD) {
                list();
            } else if (cmd == EXIT_CMD) {
                list();
                exit(0);
            } else {
                printf("ERROR: invalid Controller command: %s\n"
                       "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
            }
        }

        /*
         * 2. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         *
         *    TODO: In addition,  upon receiving signal USER1, the switch displays the information specified by the list command
         */
        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
            if (pfds[i].revents & POLLIN) {
                printf("pfds[%lu] has connection POLLIN event: %s\n", i,
                       connections[i - 1].getReceiveFIFOName().c_str());
                int r = read(pfds[i].fd, buf, 1024);
                if (!r) {
                    printf("WARNING: receiveFIFO closed\n");
                }
                string cmd = string(buf);

                // TODO: debug
                printf("DEBUG: Received output: %s\n", cmd.c_str());

                // take the message and parse it into a packet
                Packet packet = Packet(cmd);
                string packetType = packet.getType();
                Message packetMessage = packet.getMessage();
                printf("Parsed packet: %s\n", packet.toString().c_str());

                if (packet.getType() == OPEN) {
                    rOpenCount++;
                    // Upon receiving an OPEN packet, the controller updates its stored information about the switch,
                    // and replies with a packet of type ACK
                    uint switchId = static_cast<uint>(stoi(get<1>(packetMessage[0])));
                    uint switchNeighbors = static_cast<uint>(stoi(get<1>(packetMessage[1])));
                    uint switchIPLow = static_cast<uint>(stoi(get<1>(packetMessage[2])));
                    uint switchIPHigh = static_cast<uint>(stoi(get<1>(packetMessage[3])));
                    printf("Parsed OPEN packet: switchID: %u switchNeighbors: %u switchIPLow: %u switchIPHigh: %u\n",
                           switchId, switchNeighbors, switchIPLow, switchIPHigh);

                    switches.emplace_back(Switch(switchId, switchNeighbors, switchIPLow, switchIPHigh));

                    // send ack back to switch
                    printf("Sending ACK back to switchID: %u", switchId);
                    Packet ackPacket = Packet(ACK, Message());
                    write(connections[i - 1].openSendFIFO(), ackPacket.toString().c_str(),
                          strlen(ackPacket.toString().c_str()));
                    tAckCount++;
                } else if (packet.getType() == QUERY) {
                    rQueryCount++;
                    // When processing an incoming packet header (the header may be read from
                    //the traffic file, or relayed to the switch by one of its neighbours), if a switch does not find
                    //a matching rule in the flow table, the switch sends a
                    //QUERY
                    //packet to the controller.  The
                    //controller replies with a rule stored in a packet of type
                    uint switchId = static_cast<uint>(stoi(get<1>(packetMessage[0])));
                    uint srcIP = static_cast<uint>(stoi(get<1>(packetMessage[1])));
                    uint dstIP = static_cast<uint>(stoi(get<1>(packetMessage[2])));
                    printf("Parsed QUERY packet: switchId: %u srcIP: %u dstIP: %u",
                           switchId, srcIP, dstIP);

                    // calculate new flow entry
                    FlowEntry flowEntry = makeRule(switchId, srcIP, dstIP);

                    // create new add packet
                    Message addMessage;
                    addMessage.emplace_back(MessageArg("srcIP_lo", to_string(flowEntry.srcIP_lo)));
                    addMessage.emplace_back(MessageArg("srcIP_hi", to_string(flowEntry.srcIP_lo)));
                    addMessage.emplace_back(MessageArg("dstIP_lo", to_string(flowEntry.dstIP_lo)));
                    addMessage.emplace_back(MessageArg("dstIP_hi", to_string(flowEntry.dstIP_hi)));
                    addMessage.emplace_back(MessageArg("actionType", to_string(flowEntry.actionType)));
                    addMessage.emplace_back(MessageArg("actionVal", to_string(flowEntry.actionVal)));
                    addMessage.emplace_back(MessageArg("pri", to_string(flowEntry.pri)));
                    addMessage.emplace_back(MessageArg("pktCount", to_string(flowEntry.pktCount)));
                    Packet addPacket = Packet(ADD, addMessage);
                    write(connections[i - 1].openSendFIFO(), addPacket.toString().c_str(),
                          strlen(addPacket.toString().c_str()));
                    tAddCount++;
                } else {
                    if (packet.getType() == ACK) {
                        rAckCount++;
                    } else if (packet.getType() == ADD) {
                        rAddCount++;
                    } else if (packet.getType() == RELAY) {
                        // controller does nothing on ack, add, and relay
                        rRelayCount++;
                    }
                    printf("ERROR: unexpected %s packet received: %s\n", packet.getType().c_str(), cmd.c_str());
                }
            }
        }
    }
}


// TODO: doc
/**
 * Calculate a new flow entry rule.
 *
 * @param switchId
 * @param srcIP
 * @param dstIP
 * @return
 */
FlowEntry Controller::makeRule(uint switchId, uint srcIP, uint dstIP) {
    auto it = find_if(switches.begin(), switches.end(), [&switchId](Switch &sw) { return sw.getId() == switchId; });
    if (it != switches.end()) {
        // found element. it is an iterator to the first matching element.
        auto index = std::distance(switches.begin(), it);
        Switch requestSwitch = switches[index];
        // check src ip is invalid
        if (srcIP < 0 || srcIP > MAX_IP){
            // src ip is invalid make a drop rule
            // TODO: create DROP rule
            FlowEntry drop_rule = {
                    .srcIP_lo   = srcIP,
                    .srcIP_hi   = srcIP,
                    .dstIP_lo   = dstIP,
                    .dstIP_hi   = dstIP,
                    .actionType = DROP,
                    .actionVal  = 0,
                    .pri        = MIN_PRI,
                    .pktCount   = 0
            };
            return drop_rule;
        } else {
            // dst is out of range of switches range
            // make a drop rule
            if (dstIP < requestSwitch.getIpLow() || dstIP > requestSwitch.getIpHigh()) { // out of range of
                // get left switch
                int leftSwitchId = requestSwitch.getLeftSwitchId();
                if (leftSwitchId > 0) {
                    auto it2 = find_if(switches.begin(), switches.end(),
                                       [&leftSwitchId](Switch &sw) { return sw.getId() == leftSwitchId; });
                    if (it != switches.end()) {
                        auto index2 = std::distance(switches.begin(), it2);
                        Switch requestLeftSwitch = switches[index2];
                        if (dstIP < requestLeftSwitch.getIpLow() || dstIP > requestLeftSwitch.getIpHigh()) {
                        } else {
                            // TODO: make FORWARD rule
                            FlowEntry forwardLeftRule = {
                                    .srcIP_lo   = 0,
                                    .srcIP_hi   = MAX_IP,
                                    .dstIP_lo   = dstIP,
                                    .dstIP_hi   = dstIP,
                                    .actionType = FORWARD,
                                    .actionVal  = requestLeftSwitch.getId(),
                                    .pri        = MIN_PRI,
                                    .pktCount   = 0
                            };
                            return forwardLeftRule;
                        }
                    }
                }
                // get the right switch
                int rightSwitch = requestSwitch.getRightSwitchId();
                if (rightSwitch > 0) {
                    auto it3 = find_if(switches.begin(), switches.end(),
                                       [&rightSwitch](Switch &sw) { return sw.getId() == rightSwitch; });
                    if (it != switches.end()) {
                        auto index3 = std::distance(switches.begin(), it3);
                        Switch requestRightSwitch = switches[index3];
                        if (dstIP < requestRightSwitch.getIpLow() || dstIP > requestRightSwitch.getIpHigh()) {
                        } else {
                            // TODO: make FORWARD rule
                            FlowEntry forwardRightRule = {
                                    .srcIP_lo   = 0,
                                    .srcIP_hi   = MAX_IP,
                                    .dstIP_lo   = dstIP,
                                    .dstIP_hi   = dstIP,
                                    .actionType = FORWARD,
                                    .actionVal  = requestRightSwitch.getId(),
                                    .pri        = MIN_PRI,
                                    .pktCount   = 0
                            };
                            return forwardRightRule;
                        }
                    }
                }
                // all other options exhausted
                // TODO: make DROP rule
                FlowEntry drop_rule = {
                        .srcIP_lo   = 0,
                        .srcIP_hi   = MAX_IP,
                        .dstIP_lo   = dstIP,
                        .dstIP_hi   = dstIP,
                        .actionType = DROP,
                        .actionVal  = 0,
                        .pri        = MIN_PRI,
                        .pktCount   = 0
                };
                return drop_rule;
            } else {
                // TODO: make DELIVER rule
                FlowEntry deliver_rule = {
                        .srcIP_lo   = 0,
                        .srcIP_hi   = MAX_IP,
                        .dstIP_lo   = dstIP,
                        .dstIP_hi   = dstIP,
                        .actionType = DELIVER,
                        .actionVal  = 3,
                        .pri        = MIN_PRI,
                        .pktCount   = 0
                };
                return deliver_rule;
            }
        }
    } else {
        // no switch with that ID is found.
        printf("ERROR: attempted to make rule for switch that is not supported: switchId: %u", switchId);
        // TODO: make DROP rule
        // TODO: is this okay behavoir
        FlowEntry drop_rule = {
                .srcIP_lo   = 0,
                .srcIP_hi   = MAX_IP,
                .dstIP_lo   = dstIP,
                .dstIP_hi   = dstIP,
                .actionType = DROP,
                .actionVal  = 0,
                .pri        = MIN_PRI,
                .pktCount   = 0
        };
        return drop_rule;
    }
}