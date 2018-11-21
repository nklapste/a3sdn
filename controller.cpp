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
#include <fcntl.h>

/*FIFO stuff*/
#include <poll.h>

/*socket stuff*/
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "controller.h"

#define LIST_CMD "list"
#define EXIT_CMD "exit"

#define BUFFER_SIZE 1024

#define PDFS_SIZE connections.size()+4
#define PDFS_STDIN 0
#define PDFS_SIGNAL connections.size()+2
#define PDFS_SOCKET connections.size()+3

using namespace std;


/**
 * Initialize a Controller.
 *
 * @param nSwitches {@code uint} the number of switches to potentially be connected to the controller.
 */
Controller::Controller(uint nSwitches, Port port) : nSwitches(nSwitches), Gate(port) {
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

    printf("INFO: created controller: nSwitches: %u portNumber: %u\n", nSwitches, port.getPortNum());
}

/**
 * Print the switches connected to the controller and the statistics of packets sent and received.
 */
void Controller::list() {
    listControllerStats();
    listPacketStats();
}

/**
 * Start the {@code Controller} loop.
 */
void Controller::start() {
    struct pollfd pfds[PDFS_SIZE];

    // TODO: is this needed now?
    // setup file descriptions or stdin and all connection FIFOs
    pfds[PDFS_STDIN].fd = STDIN_FILENO;

    pfds[PDFS_SIGNAL].fd = getSignalFD();

    // TODO: init TCP socket connection
    // init TCP socket file descriptor
    struct sockaddr_in address{};
    int opt = 1;

    // Creating socket file descriptor
    if ((pfds[PDFS_SOCKET].fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("ERROR: creating socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(pfds[PDFS_SOCKET].fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("ERROR: attaching socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(getPort().getPortNum());

    // Forcefully attaching socket to the port 8080
    if (bind(pfds[PDFS_SOCKET].fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(pfds[PDFS_SOCKET].fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //TODO: dev
    int newsockfd;
    int addrlen = sizeof(address);

    // enter the controller loop
    for (;;) {
        // setup for the next round of polling
        for (auto &pfd : pfds) {
            pfd.events = POLLIN;
        }
        int ret = poll(pfds, PDFS_SIZE, 0);
        if (errno || ret < 0) {
            perror("ERROR: poll failure");
            exit(errno);
        }

        /*
         * Check stdin
         */
        if (pfds[PDFS_STDIN].revents & POLLIN) {
            check_stdin(pfds[PDFS_STDIN].fd);
        }

        /*
         * Check signals
         */
        if (pfds[PDFS_SIGNAL].revents & POLLIN) {
            check_signal(pfds[PDFS_SIGNAL].fd);
        }

        /*
         * Check the socket file descriptor for events.
         *
         * This is how we add new TCP socket client connections.
         */
        if (pfds[PDFS_SOCKET].revents & POLLIN) {
            if (clientSocketConnections.size() + 1 > nSwitches) {
                printf("WARNING: skipping adding TCP client socket connection that would exceed nSwitches");
            } else {
                if ((newsockfd = accept(pfds[PDFS_SOCKET].fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) <
                    0) {
                    perror("ERROR: calling server accept");
                    exit(EXIT_FAILURE);
                }
                printf("INFO: new client connection: socket fd:%d ip:%s port:%hu\n",
                       newsockfd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                if (fcntl(newsockfd, F_SETFL, fcntl(newsockfd, F_GETFL) | O_NONBLOCK) < 0) {
                    // handle error
                    perror("ERROR: setting to non block socket\n");
                }
                clientSocketConnections.emplace_back(
                        make_tuple(newsockfd, (char *) malloc(BUFFER_SIZE * sizeof(char))));
            }
        }
        int i = 0;
        for (tuple<int, char *> &tup : clientSocketConnections) {
            // TODO: sometime query packet is missed
            if (recv(get<0>(tup), get<1>(tup), sizeof(get<1>(tup)), MSG_PEEK | MSG_DONTWAIT) == 0) {
                //Somebody disconnected , get his details and print
                getpeername(get<0>(tup), (struct sockaddr *) &address, (socklen_t *) &addrlen);
                printf("INFO: switch disconnected: fd:%d ip:%s port:%d\n",
                       get<0>(tup), inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                close(get<0>(tup));
                clientSocketConnections.erase(clientSocketConnections.begin() + i);
                // TODO: remove switch from switches?

                free(get<1>(tup));
            } else {
                check_sock(
                        get<0>(tup),
                        get<1>(tup)
                );
            }
            i++;
        }
    }
}


/**
 * Check the socket file descriptor for events
 */
void Controller::check_sock(int socketFD, char *tmpbuf) {
    string msg = get_message(socketFD, tmpbuf);

    Packet packet = Packet(msg);
    if (errno == EINVAL) {
        errno = 0;
    } else {
        printf("DEBUG: parsed packet: %s\n", packet.toString().c_str());
        if (packet.getType() == OPEN) {
            respondOPENPacket(socketFD, packet.getMessage());
        } else if (packet.getType() == QUERY) {
            respondQUERYPacket(socketFD, packet.getMessage());
        } else {
            // Controller has no other special behavior for other packets
            if (packet.getType() == ACK) {
                rAckCount++;
            } else if (packet.getType() == ADD) {
                rAddCount++;
            } else if (packet.getType() == RELAY) {
                rRelayCount++;
            }
            printf("ERROR: unexpected %s packet received: %s\n", packet.getType().c_str(), msg.c_str());
        }
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
FlowEntry Controller::makeFlowEntry(SwitchID switchID, uint srcIP, uint dstIP) {
    printf("DEBUG: making FlowEntry for: switchID: %s srcIP: %u dstIP: %u\n",
           switchID.getSwitchIDString().c_str(), srcIP, dstIP);
    auto it = find_if(switches.begin(), switches.end(),
                      [&switchID](Switch &sw) { return sw.getGateID() == switchID.getSwitchIDNum(); });
    if (it != switches.end()) {
        printf("DEBUG: found matching switch: %s\n", switchID.getSwitchIDString().c_str());

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
                SwitchID leftSwitchID = requestSwitch.getLeftSwitchID();
                printf("DEBUG: checking leftSwitch: %s\n", leftSwitchID.getSwitchIDString().c_str());
                if (!leftSwitchID.isNullSwitchID()) {
                    auto it2 = find_if(switches.begin(), switches.end(),
                                       [&leftSwitchID](Switch &sw) {
                                           return sw.getGateID() == leftSwitchID.getSwitchIDNum();
                                       });
                    if (it2 != switches.end()) {
                        auto index2 = std::distance(switches.begin(), it2);
                        Switch requestLeftSwitch = switches[index2];
                        printf("DEBUG: found leftSwitch: %s ipLOW: %u ipHIGH: %u\n",
                               requestLeftSwitch.getSwitchID().getSwitchIDString().c_str(),
                               requestLeftSwitch.getIPLow(), requestLeftSwitch.getIPHigh());
                        if (dstIP < requestLeftSwitch.getIPLow() || dstIP > requestLeftSwitch.getIPHigh()) {
                        } else {
                            printf("DEBUG: left switch valid creating FORWARD FlowEntry\n");
                            FlowEntry forwardLeftRule = {
                                    .srcIPLow   = MIN_IP,
                                    .srcIPHigh   = MAX_IP,
                                    .dstIPLow   = requestLeftSwitch.getIPLow(),
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
                SwitchID rightSwitchID = requestSwitch.getRightSwitchID();
                printf("DEBUG: checking rightSwitch: %s\n", rightSwitchID.getSwitchIDString().c_str());
                if (!rightSwitchID.isNullSwitchID()) {

                    auto it3 = find_if(switches.begin(), switches.end(),
                                       [&rightSwitchID](Switch &sw) {
                                           return sw.getGateID() == rightSwitchID.getSwitchIDNum();
                                       });

                    if (it3 != switches.end()) {

                        auto index3 = std::distance(switches.begin(), it3);

                        Switch requestRightSwitch = switches[index3];

                        printf("DEBUG: found rightSwitch: %s ipLOW: %u ipHIGH: %u\n",
                               requestRightSwitch.getSwitchID().getSwitchIDString().c_str(),
                               requestRightSwitch.getIPLow(), requestRightSwitch.getIPHigh());

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
        printf("ERROR: attempted to make FlowEntry for unexpected switch: %s creating DROP FlowEntry\n",
               switchID.getSwitchIDString().c_str());
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
 * @param socketFD {@code int}
 */
void Controller::sendACKPacket(int socketFD) {
    Packet ackPacket = Packet(ACK, Message());
    // TODO: fix log statement
    printf("INFO: sending ACK packet: packet: %s\n", ackPacket.toString().c_str());
    send(socketFD, ackPacket.toString().c_str(), strlen(ackPacket.toString().c_str())+1, 0);
    tAckCount++;
}

/**
 * Create and send a ADD packet out to the specified TCP socket connection.
 * Indicating to add the specified FlowEntry.
 *
 * @param socketFD {@code int}
 * @param flowEntry {@code FlowEntry}
 */
void Controller::sendADDPacket(int socketFD, FlowEntry flowEntry) {
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
    printf("INFO: sending ADD packet: packet: %s\n", addPacket.toString().c_str());
    send(socketFD, addPacket.toString().c_str(), strlen(addPacket.toString().c_str())+1, 0);
    tAddCount++;
}

/**
 * Respond to a OPEN packet.
 *
 * Upon receiving an OPEN packet, the controller updates its stored information about the switch,
 * and replies with a packet of type ACK.
 *
 * @param socketfd {@code int}
 * @param message {@code Message}
 */
void Controller::respondOPENPacket(int socketfd, Message message) {
    rOpenCount++;
    SwitchID switchID = SwitchID(get<1>(message[0]));
    SwitchID leftSwitchID = SwitchID(get<1>(message[1]));
    SwitchID rightSwitchID = SwitchID(get<1>(message[2]));
    uint switchIPLow = static_cast<uint>(stoi(get<1>(message[3])));
    uint switchIPHigh = static_cast<uint>(stoi(get<1>(message[4])));
    Address address = Address(get<1>(message[5]));
    Port port = Port(static_cast<u_int16_t>(stoi(get<1>(message[6]))));
    printf("DEBUG: parsed OPEN packet: switchID: %s leftSwitchID: %s rightSwitchID: %s switchIPLow: %u switchIPHigh: %u address: %s port: %u\n",
           switchID.getSwitchIDString().c_str(), leftSwitchID.getSwitchIDString().c_str(),
           rightSwitchID.getSwitchIDString().c_str(),
           switchIPLow, switchIPHigh, address.getSymbolicName().c_str(),
           port.getPortNum());

    // make sure that the switch is within the controllers specified number of switches
    if (switchID.getSwitchIDNum()>nSwitches ||
        leftSwitchID.getSwitchIDNum()>nSwitches ||
        rightSwitchID.getSwitchIDNum()>nSwitches){
        printf("ERROR: shutting down controller: OPEN packet contains switchid outside of the controllers max number of switches: %u\n",
               nSwitches);
        // TODO: better way maybe?
        exit(EINVAL);
    } else {
        // create the new switch from the parsed OPEN packet
        Switch newSwitch = Switch(switchID, leftSwitchID, rightSwitchID, switchIPLow, switchIPHigh, address, port);

        // check if we are creating or updating a switch
        auto it = find_if(switches.begin(), switches.end(), [&newSwitch](Switch &sw) {
            return sw.getGateID() ==
                   newSwitch.getGateID();
        });
        if (it != switches.end()) {
            printf("DEBUG: updating existing switch: switchID: %u leftSwitchID: %i rightSwitchID: %i switchIPLow: %u switchIPHigh: %u\n",
                   newSwitch.getGateID(), newSwitch.getRightSwitchID().getSwitchIDNum(),
                   newSwitch.getLeftSwitchID().getSwitchIDNum(), newSwitch.getIPLow(),
                   newSwitch.getIPHigh());
            auto index = std::distance(switches.begin(), it);
            switches[index] = newSwitch;
        } else {
            printf("DEBUG: adding new switch: switchID: %u leftSwitchID: %i rightSwitchID: %i switchIPLow: %u switchIPHigh: %u\n",
                   newSwitch.getGateID(), newSwitch.getRightSwitchID().getSwitchIDNum(),
                   newSwitch.getLeftSwitchID().getSwitchIDNum(), newSwitch.getIPLow(),
                   newSwitch.getIPHigh());
            // add the switch to the controllers list of known switches
            switches.emplace_back(newSwitch);
        }

        // sort and dedupe the list of switches
        sort(switches.begin(), switches.end());
        switches.erase(unique(switches.begin(), switches.end()), switches.end());

        // send ack back to switch
        sendACKPacket(socketfd);
    }
}

/**
 * Respond to a QUERY packet.
 *
 * @param socketFD {@code int}
 * @param message {@code Message}
 */
void Controller::respondQUERYPacket(int socketFD, Message message) {
    rQueryCount++;
    SwitchID switchID = SwitchID(get<1>(message[0]));
    uint srcIP = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP = static_cast<uint>(stoi(get<1>(message[2])));
    printf("DEBUG: parsed QUERY packet: switchID: %s srcIP: %u dstIP: %u\n",
           switchID.getSwitchIDString().c_str(), srcIP, dstIP);

    // calculate new flow entry
    FlowEntry flowEntry = makeFlowEntry(switchID, srcIP, dstIP);

    // create and send new add packet
    sendADDPacket(socketFD, flowEntry);
}

/**
 * Print {@code Controller} specific statistics.
 */
void Controller::listControllerStats() {
    printf("Controller information:\n");
    for (auto &sw: switches) {
        printf("[%s] port1= %s, port2= %s, port3= %u-%u\n",
               sw.getSwitchID().getSwitchIDString().c_str(),
               sw.getLeftSwitchID().getSwitchIDString().c_str(),
               sw.getRightSwitchID().getSwitchIDString().c_str(),
               sw.getIPLow(), sw.getIPHigh());
    }
}
