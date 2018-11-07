/**
 * a2sdn controller.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>

/*FIFO stuff*/
#include <poll.h>

#include "controller.h"

#define LIST_CMD "list"
#define EXIT_CMD "exit"

#define BUFFER_SIZE 1024

#define PDFS_SIZE connections.size()+3
#define PDFS_STDIN 0
#define PDFS_SIGNAL connections.size()+2

using namespace std;

/**
 * Initialize a Controller.
 *
 * @param nSwitches {@code uint} the number of switches to potentially be connected to the controller.
 */
Controller::Controller(uint nSwitches) : nSwitches(nSwitches) {
    if (nSwitches > MAX_SWITCHES) {
        printf("ERROR: too many switches for controller: %u\n"
               "\tMAX_SWITCHES=%u\n", nSwitches, MAX_SWITCHES);
        exit(EINVAL);
    }
    if (nSwitches < MIN_SWITCHES) {
        printf("ERROR: too little switches for controller: %u\n"
               "\tMIN_SWITCHES=%u\n", nSwitches, MIN_SWITCHES);
        exit(EINVAL);
    }
    printf("DEBUG: creating controller: nSwitches: %u\n", nSwitches);

    // init all potential switch connections for the controller
    for (uint switch_i = 1; switch_i <= nSwitches; ++switch_i) {
        connections.emplace_back(CONTROLLER_ID, switch_i);
    }
    printf("INFO: created controller: nSwitches: %u\n", nSwitches);
}

/**
 * Print the switches connected to the controller and the statistics of packets sent and received.
 */
void Controller::list() {
    printf("Controller information:\n");
    for (auto &sw: switches) {
        printf("[sw%u] port1= %i, port2= %i, port3= %u-%u\n",
               sw.getGateID(), sw.getLeftSwitchID(), sw.getRightSwitchID(), sw.getIPLow(), sw.getIPHigh());
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
    struct pollfd pfds[PDFS_SIZE];
    char buf[BUFFER_SIZE];

    // setup file descriptions or stdin and all connection FIFOs
    pfds[PDFS_STDIN].fd = STDIN_FILENO;
    for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
        pfds[i].fd = connections[i - 1].openReceiveFIFO();
    }

    sigset_t sigset;
    /* Create a sigset of all the signals that we're interested in */
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    /* We must block the signals in order for signalfd to receive them */
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    if (errno) {
        perror("ERROR: setting signal mask");
        exit(errno);
    }

    /* This is the main loop */
    pfds[PDFS_SIGNAL].fd = signalfd(-1, &sigset, 0);;

    // enter the controller loop
    for (;;) {
        /*
         * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        for (auto &pfd : pfds) {
            pfd.events = POLLIN;
        }
        int ret = poll(pfds, PDFS_SIZE, 0);
        if (errno || ret < 0) {
            perror("ERROR: poll failure");
            exit(errno);
        }
        if (pfds[PDFS_STDIN].revents & POLLIN) {
            ssize_t r = read(pfds[PDFS_STDIN].fd, buf, BUFFER_SIZE);
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
                printf("INFO: exit command received: terminating\n");
                exit(0);
            } else {
                printf("ERROR: invalid Controller command: %s\n"
                       "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
            }
        }

        /*
         * 2. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         */
        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
            if (pfds[i].revents & POLLIN) {
                printf("DEBUG: pfds[%lu] has connection POLLIN event: %s\n", i,
                       connections[i - 1].getReceiveFIFOName().c_str());
                ssize_t r = read(pfds[i].fd, buf, BUFFER_SIZE);
                if (!r) {
                    printf("WARNING: receiveFIFO closed\n");
                }
                string cmd = string(buf);

                // take the message and parse it into a packet
                Packet packet = Packet(cmd);
                printf("DEBUG: parsed packet: %s\n", packet.toString().c_str());

                if (packet.getType() == OPEN) {
                    respondOPENPacket(connections[i - 1], packet.getMessage());
                } else if (packet.getType() == QUERY) {
                    respondQUERYPacket(connections[i - 1], packet.getMessage());
                } else {
                    // Controller has no other special behavior for other packets
                    if (packet.getType() == ACK) {
                        rAckCount++;
                    } else if (packet.getType() == ADD) {
                        rAddCount++;
                    } else if (packet.getType() == RELAY) {
                        rRelayCount++;
                    }
                    printf("ERROR: unexpected %s packet received: %s\n", packet.getType().c_str(), cmd.c_str());
                }
            }
        }

        /*
         * In addition, upon receiving signal USER1, the switch displays the information specified by the list command.
         */
        if (pfds[PDFS_SIGNAL].revents & POLLIN) {
            struct signalfd_siginfo info{};
            /* We have a valid signal, read the info from the fd */
            ssize_t r = read(pfds[PDFS_SIGNAL].fd, &info, sizeof(info));
            if (!r) {
                printf("WARNING: signal reading error\n");
            }
            unsigned sig = info.ssi_signo;
            if (sig == SIGUSR1) {
                printf("DEBUG: received SIGUSR1 signal\n");
                list();
            }
        }
        // clear buffer
        memset(buf, 0, sizeof buf);
    }
}

/**
 * Calculate a new flow entry rule.
 *
 * TODO: This code is spaghetti. Someone needs to eat it.
 *
 * @param switchID {@code uint}
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 * @return {@code FlowEntry}
 */
FlowEntry Controller::makeFlowEntry(uint switchID, uint srcIP, uint dstIP) {
    printf("DEBUG: making FlowEntry for: switchID: %u srcIP: %u dstIP: %u\n",
           switchID, srcIP, dstIP);
    auto it = find_if(switches.begin(), switches.end(), [&switchID](Switch &sw) { return sw.getGateID() == switchID; });
    if (it != switches.end()) {
        printf("DEBUG: found matching switch: %u\n", switchID);

        // found element. it is an iterator to the first matching element.
        auto index = std::distance(switches.begin(), it);
        Switch requestSwitch = switches[index];

        // check srcIP is invalid
        if (srcIP < 0 || srcIP > MAX_IP) {
            printf("DEBUG: invalid srcIP: %u creating DROP FlowEntry\n", srcIP);
            FlowEntry drop_rule = {
                    .srcIPLow   = MIN_IP,
                    .srcIPHigh   = MAX_IP,
                    .dstIPLow   = dstIP,
                    .dstIPHigh   = dstIP,
                    .actionType = DROP,
                    .actionVal  = PORT_0,
                    .pri        = MIN_PRI,
                    .pktCount   = 0
            };
            return drop_rule;
        } else {
            printf("DEBUG: valid srcIP: %u\n", srcIP);
            if (dstIP < requestSwitch.getIPLow() || dstIP > requestSwitch.getIPHigh()) { // out of range of
                printf("DEBUG: invalid dstIP: %u checking if neighboring switches are valid\n", dstIP);

                // get and test left switch
                int leftSwitchID = requestSwitch.getLeftSwitchID();
                printf("DEBUG: checking leftSwitchID: %i\n", leftSwitchID);
                if (leftSwitchID > 0) {
                    auto it2 = find_if(switches.begin(), switches.end(),
                                       [&leftSwitchID](Switch &sw) { return sw.getGateID() == leftSwitchID; });
                    if (it2 != switches.end()) {
                        auto index2 = std::distance(switches.begin(), it2);
                        Switch requestLeftSwitch = switches[index2];
                        if (dstIP < requestLeftSwitch.getIPLow() || dstIP > requestLeftSwitch.getIPHigh()) {
                        } else {
                            printf("DEBUG: left switch valid creating FORWARD FlowEntry\n");
                            FlowEntry forwardLeftRule = {
                                    .srcIPLow   = MIN_IP,
                                    .srcIPHigh   = MAX_IP,
                                    .dstIPLow   = requestLeftSwitch.getIPLow() ,
                                    .dstIPHigh   = requestLeftSwitch.getIPHigh(),
                                    .actionType = FORWARD,
                                    .actionVal  = PORT_1,
                                    .pri        = MIN_PRI,
                                    .pktCount   = 0,
                            };
                            return forwardLeftRule;
                        }
                    }
                }
                // get and test the right switch
                int rightSwitchID = requestSwitch.getRightSwitchID();
                printf("DEBUG: checking rightSwitchID: %i\n", rightSwitchID);
                if (rightSwitchID > 0) {

                    auto it3 = find_if(switches.begin(), switches.end(),
                                       [&rightSwitchID](Switch &sw) { return sw.getGateID() == rightSwitchID; });

                    if (it3 != switches.end()) {

                        auto index3 = std::distance(switches.begin(), it3);

                        Switch requestRightSwitch = switches[index3];

                        if (dstIP < requestRightSwitch.getIPLow() || dstIP > requestRightSwitch.getIPHigh()) {

                        } else {
                            printf("DEBUG: right switch valid creating FORWARD FlowEntry\n");
                            FlowEntry forwardRightRule = {
                                    .srcIPLow   = MIN_IP,
                                    .srcIPHigh   = MAX_IP,
                                    .dstIPLow   = requestRightSwitch.getIPLow(),
                                    .dstIPHigh   = requestRightSwitch.getIPHigh(),
                                    .actionType = FORWARD,
                                    .actionVal  = PORT_2,
                                    .pri        = MIN_PRI,
                                    .pktCount   = 0,
                            };
                            return forwardRightRule;
                        }
                    }
                }
                printf("DEBUG: left and right switch invalid creating DROP FlowEntry\n");
                FlowEntry drop_rule = {
                        .srcIPLow   = MIN_IP,
                        .srcIPHigh   = MAX_IP,
                        .dstIPLow   = dstIP,
                        .dstIPHigh   = dstIP,
                        .actionType = DROP,
                        .actionVal  = PORT_0,
                        .pri        = MIN_PRI,
                        .pktCount   = 0
                };
                return drop_rule;
            } else {
                printf("DEBUG: valid dstIP: %u creating DELIVER FlowEntry\n", dstIP);
                FlowEntry deliver_rule = {
                        .srcIPLow   = MIN_IP,
                        .srcIPHigh   = MAX_IP,
                        .dstIPLow   = dstIP,
                        .dstIPHigh   = dstIP,
                        .actionType = DELIVER,
                        .actionVal  = PORT_3,
                        .pri        = MIN_PRI,
                        .pktCount   = 0
                };
                return deliver_rule;
            }
        }
    } else {
        printf("ERROR: attempted to make FlowEntry for unexpected switch: sw%u creating DROP FlowEntry\n", switchID);
        // TODO: is this okay behavior
        FlowEntry drop_rule = {
                .srcIPLow   = MIN_IP,
                .srcIPHigh   = MAX_IP,
                .dstIPLow   = dstIP,
                .dstIPHigh   = dstIP,
                .actionType = DROP,
                .actionVal  = PORT_0,
                .pri        = MIN_PRI,
                .pktCount   = 0
        };
        return drop_rule;
    }
}

/**
 * Create and send a ACK packet out to the specified connection.
 *
 * @param connection {@code Connection}
 */
void Controller::sendACKPacket(Connection connection) {
    Packet ackPacket = Packet(ACK, Message());
    printf("INFO: sending ACK packet: connection: %s packet: %s\n",
           connection.getSendFIFOName().c_str(), ackPacket.toString().c_str());
    write(connection.openSendFIFO(), ackPacket.toString().c_str(),
          strlen(ackPacket.toString().c_str()));
    tAckCount++;
}

/**
 * Create and send a ADD packet out to the specified connection. Indicating to add the specified FlowEntry.
 *
 * @param connection {@code Connection}
 * @param flowEntry {@code FlowEntry}
 */
void Controller::sendADDPacket(Connection connection, FlowEntry flowEntry) {
    Message addMessage;
    addMessage.emplace_back(MessageArg("srcIPLow", to_string(flowEntry.srcIPLow)));
    addMessage.emplace_back(MessageArg("srcIPHigh", to_string(flowEntry.srcIPHigh)));
    addMessage.emplace_back(MessageArg("dstIPLow", to_string(flowEntry.dstIPLow)));
    addMessage.emplace_back(MessageArg("dstIPHigh", to_string(flowEntry.dstIPHigh)));
    addMessage.emplace_back(MessageArg("actionType", to_string(flowEntry.actionType)));
    addMessage.emplace_back(MessageArg("actionVal", to_string(flowEntry.actionVal)));
    addMessage.emplace_back(MessageArg("pri", to_string(flowEntry.pri)));
    addMessage.emplace_back(MessageArg("pktCount", to_string(flowEntry.pktCount)));
    Packet addPacket = Packet(ADD, addMessage);
    printf("INFO: sending ADD packet: connection: %s packet: %s\n",
           connection.getSendFIFOName().c_str(), addPacket.toString().c_str());
    write(connection.openSendFIFO(), addPacket.toString().c_str(),
          strlen(addPacket.toString().c_str()));
    tAddCount++;
}

/**
 * Respond to a OPEN packet.
 *
 * Upon receiving an OPEN packet, the controller updates its stored information about the switch,
 * and replies with a packet of type ACK.
 *
 * @param connection {@code Connection}
 * @param message {@code Message}
 */
void Controller::respondOPENPacket(Connection connection, Message message) {
    rOpenCount++;
    uint switchID     = static_cast<uint>(stoi(get<1>(message[0])));
    int leftSwitchID  =                   stoi(get<1>(message[1]));
    int rightSwitchID =                   stoi(get<1>(message[2]));
    uint switchIPLow  = static_cast<uint>(stoi(get<1>(message[3])));
    uint switchIPHigh = static_cast<uint>(stoi(get<1>(message[4])));
    printf("DEBUG: parsed OPEN packet: switchID: %u leftSwitchID: %i rightSwitchID: %i switchIPLow: %u switchIPHigh: %u\n",
           switchID, leftSwitchID, rightSwitchID, switchIPLow, switchIPHigh);

    // create the new switch from the parsed OPEN packet
    Switch newSwitch = Switch(switchID, leftSwitchID, rightSwitchID, switchIPLow, switchIPHigh);

    // check if we are creating or updating a switch
    auto it = find_if(switches.begin(), switches.end(), [&newSwitch](Switch &sw) {
        return sw.getGateID() ==
               newSwitch.getGateID();
    });
    if (it != switches.end()) {
        printf("DEBUG: updating existing switch: switchID: %u leftSwitchID: %i rightSwitchID: %i switchIPLow: %u switchIPHigh: %u\n",
               newSwitch.getGateID(), newSwitch.getRightSwitchID(), newSwitch.getLeftSwitchID(), newSwitch.getIPLow(),
               newSwitch.getIPHigh());
        auto index = std::distance(switches.begin(), it);
        switches[index] = newSwitch;
    } else {
        printf("DEBUG: adding new switch: switchID: %u leftSwitchID: %i rightSwitchID: %i switchIPLow: %u switchIPHigh: %u\n",
               newSwitch.getGateID(), newSwitch.getRightSwitchID(), newSwitch.getLeftSwitchID(), newSwitch.getIPLow(),
               newSwitch.getIPHigh());
        // add the switch to the controllers list of known switches
        switches.emplace_back(newSwitch);
    }

    // sort and dedupe the list of switches
    sort(switches.begin(), switches.end());
    switches.erase(unique(switches.begin(), switches.end()), switches.end());

    // send ack back to switch
    sendACKPacket(std::move(connection));
}

/**
 * Respond to a QUERY packet.
 *
 * @param connection {@code Connection}
 * @param message {@code Message}
 */
void Controller::respondQUERYPacket(Connection connection, Message message) {
    rQueryCount++;
    uint switchID = static_cast<uint>(stoi(get<1>(message[0])));
    uint srcIP    = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP    = static_cast<uint>(stoi(get<1>(message[2])));
    printf("DEBUG: parsed QUERY packet: switchID: %u srcIP: %u dstIP: %u\n",
           switchID, srcIP, dstIP);

    // calculate new flow entry
    FlowEntry flowEntry = makeFlowEntry(switchID, srcIP, dstIP);

    // create and send new add packet
    sendADDPacket(std::move(connection), flowEntry);
}
