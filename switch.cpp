/**
 * switch.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */


#include <chrono>
#include <ctime>
#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/* FIFO stuff */
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>

#include "controller.h"
#include "switch.h"
#include "trafficfile.h"

#define LIST_CMD "list"
#define EXIT_CMD "exit"

#define BUFFER_SIZE 1024

#define PDFS_SIZE 5
#define PDFS_STDIN 0
#define PDFS_LEFT_SWITCH 1
#define CONNECTION_LEFT_SWITCH 0
#define PDFS_RIGHT_SWITCH 2
#define CONNECTION_RIGHT_SWITCH 1
#define PDFS_SIGNAL 3
#define PDFS_SOCKET 4

using namespace std;
using namespace chrono;


/**
 * Initialize a switch.
 *
 * @param switchID {@code std::string}
 * @param leftSwitchID {@code std::string}
 * @param rightSwitchID {@code std::string}
 * @param trafficFile {@code std::string}
 * @param IPRangeStr {@code std::string}
 */
Switch::Switch(SwitchID switchID, SwitchID leftSwitchID, SwitchID rightSwitchID,
               string &trafficFile, uint IPLow, uint IPHigh, Address address, Port port) : Gate(port),
                                                                                           switchID(switchID),
                                                                                           leftSwitchID(leftSwitchID),
                                                                                           rightSwitchID(rightSwitchID),
                                                                                           IPLow(IPLow), IPHigh(IPHigh),
                                                                                           address(address) {

    // create and add the Switch's initial FLowEntry rule
    FlowEntry init_rule = {
            .srcIPLow   = MIN_IP,
            .srcIPHigh   = MAX_IP,
            .dstIPLow   = IPLow,
            .dstIPHigh   = IPHigh,
            .actionType = FORWARD,
            .actionVal  = PORT_3,
            .pri        = MIN_PRI,
            .pktCount   = 0
    };

    flowTable.push_back(init_rule);

    Switch::trafficFile = trafficFile;

    // create Connection to controller
    // TODO: needs to be changed to TCP sockets
    gateID = switchID.getSwitchIDNum();
    // create Connection to the left switch
    // can potentially be a nullptr
    if (!leftSwitchID.isNullSwitchID()) {
        connections.emplace_back(Connection(getSwitchID().getSwitchIDNum(), Switch::leftSwitchID.getSwitchIDNum()));
    } else {
        connections.emplace_back(Connection());
    }

    // create Connection to the right switch
    // can potentially be a nullptr
    if (!rightSwitchID.isNullSwitchID()) {
        connections.emplace_back(Connection(getSwitchID().getSwitchIDNum(), Switch::rightSwitchID.getSwitchIDNum()));
    } else {
        connections.emplace_back(Connection());
    }
    printf("INFO: created switch: sw%u trafficFile: %s swj: %u swk: %u IPLow: %u IPHigh: %u portNumber: %u\n",
           switchID.getSwitchIDNum(), trafficFile.c_str(), leftSwitchID.getSwitchIDNum(),
           rightSwitchID.getSwitchIDNum(), getIPLow(), getIPHigh(), port.getPortNum());
}

/**
 * Constructor by the controller to make a switch for upkeep.
 *
 * Note: this constructor not intended for actual usage by invoking the {@code start()} method.
 *
 * @param switchID {@code uint}
 * @param IPLow {@code uint}
 * @param IPHigh {@code uint}
 */
Switch::Switch(SwitchID switchID, SwitchID leftSwitchID, SwitchID rightSwitchID, uint IPLow, uint IPHigh,
               Address address, Port port) :
        switchID(switchID), leftSwitchID(leftSwitchID), rightSwitchID(rightSwitchID),
        IPLow(IPLow), IPHigh(IPHigh), address(address), Gate(port) {
    gateID = switchID.getSwitchIDNum();
}

/**
 * Getter for a switch's {@code IPLow}.
 *
 * @return {@code uint}
 */
uint Switch::getIPLow() const {
    return IPLow;
}

/**
 * Getter for a switch's {@code IPHigh}.
 *
 * @return {@code uint}
 */
uint Switch::getIPHigh() const {
    return IPHigh;
}

/**
 * Getter for a switch's {@code leftSwitchID}.
 *
 * Note: if the switch does not have a left neighboring switch {@code -1} will be returned.
 *
 * @return {@code SwitchID}
 */
SwitchID Switch::getLeftSwitchID() const {
    return leftSwitchID;
}

/**
 * Getter for a switch's {@code rightSwitchID}.
 *
 * Note: if the switch does not have a right neighboring switch {@code -1} will be returned.
 *
 * @return {@code SwitchID}
 */
SwitchID Switch::getRightSwitchID() const {
    return rightSwitchID;
}

/**
 * Start the {@code Switch} loop.
 */
void Switch::start() {
    struct pollfd pfds[PDFS_SIZE];

    // setup file descriptiors for all interswitch connection FIFOs
    pfds[PDFS_STDIN].fd = STDIN_FILENO;
    if (!leftSwitchID.isNullSwitchID()) {
        pfds[PDFS_LEFT_SWITCH].fd = connections[CONNECTION_LEFT_SWITCH].openReceiveFIFO();
    }
    if (!rightSwitchID.isNullSwitchID()) {
        pfds[PDFS_RIGHT_SWITCH].fd = connections[CONNECTION_RIGHT_SWITCH].openReceiveFIFO();
    }


    pfds[PDFS_SIGNAL].fd = getSignalFD();

    // init the trafficfile
    ifstream trafficFileStream(trafficFile);

    // init client tcp connection
    struct sockaddr_in serv_addr{};

    // Creating socket file descriptor
    if ((pfds[PDFS_SOCKET].fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("ERROR: socket file descriptor creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(getPort().getPortNum());

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, address.getIPAddr().c_str(), &serv_addr.sin_addr) <= 0) {
        perror("ERROR: invalid address");
        exit(EINVAL);
    }
    printf("INFO: connecting to: %s\n", getServerAddr().getIPAddr().c_str());
    int connection = connect(pfds[PDFS_SOCKET].fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (connection < 0) {
        perror("ERROR: connection failed");
        exit(ENOTCONN);
    }

    // When a switch starts, it sends an OPEN packet to the controller.
    // The carried message contains the switch number, the numbers of its neighbouring switches (if any),
    // and the range of IP addresses served by the switch.
    sendOPENPacket(pfds[PDFS_SOCKET].fd);

    //TODO: dev
    char buf[BUFFER_SIZE];


    if(fcntl(pfds[PDFS_SOCKET].fd, F_SETFL, fcntl(pfds[PDFS_SOCKET].fd, F_GETFL) | O_NONBLOCK) < 0) {
        // handle error
        perror("ERROR: setting to non block socket\n");
    }

    /* This is the main loop */
    for (;;) {
        // setup for the next round of polling
        for (auto &pfd : pfds) {
            pfd.events = POLLIN;
        }
        int ret = poll(pfds, PDFS_SIZE, 1);
        if (errno || ret < 0) {
            perror("ERROR: poll failure");
            exit(errno);
        }

        /*
         * Check the trafficFile
         */
        if (delayPassed()) {  // ensure that we are not experiencing a delay
            check_trafficFile(pfds[PDFS_SOCKET].fd, trafficFileStream);
        }

        /*
         * Check stdin
         */
        if (pfds[PDFS_STDIN].revents & POLLIN) {
            check_stdin(pfds[PDFS_STDIN].fd);
        }

        /*
         * Check Switch<->Switch connections
         */
        if (!leftSwitchID.isNullSwitchID()) {  // don't check connection on null switches
            check_connection(pfds[PDFS_LEFT_SWITCH].fd, pfds[PDFS_SOCKET].fd, connections[CONNECTION_LEFT_SWITCH]);
        }
        if (!rightSwitchID.isNullSwitchID()) {  // don't check connection on null switches
            check_connection(pfds[PDFS_RIGHT_SWITCH].fd, pfds[PDFS_SOCKET].fd, connections[CONNECTION_RIGHT_SWITCH]);
        }

        /*
         * Check signal
         */
        if (pfds[PDFS_SIGNAL].revents & POLLIN) {
            check_signal(pfds[PDFS_SIGNAL].fd);
        }

        /*
         * Check the TCP socket file descriptor
         */
        if (recv(pfds[PDFS_SOCKET].fd, buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT) == 0) {
            // if recv returns zero, that means the connection has been closed:
            // kill the switch
            int addrlen = sizeof(serv_addr);
            getpeername(pfds[PDFS_SOCKET].fd, (struct sockaddr *) &serv_addr, (socklen_t *) &addrlen);
            printf("ERROR: shutting down switch: controller disconnected: ip %s , port %d \n",
                   inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
            close(pfds[PDFS_SOCKET].fd);
            exit(1);
        } else {
            check_sock(pfds[PDFS_SOCKET].fd, buf);
        }
    }
}

/**
 * 1.  Read and process a single line from the traffic line (if the EOF has not been reached yet).
 *     The switch ignores empty lines, comment lines, and lines specifying other handling switches.
 *     A packet header is considered admitted if the line specifies the current switch
 *
 * Note: After reading all lines in the traffic file, the program continues to monitor and process
 *       keyboard commands, and the incoming packets from neighbouring devices.
 */
void Switch::check_trafficFile(int socketFD, ifstream &trafficFileStream) {
    string line;
    if (trafficFileStream.is_open()) {
        if (getline(trafficFileStream, line)) {
            parseTrafficFileLine(socketFD, line);
        } else {
            trafficFileStream.close();
            printf("DEBUG: finished reading traffic file\n");
        }
    }
}

/**
 * 3. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
 *    each incoming packet, as described in the Packet Types.
 */
void Switch::check_connection(int connectionFD, int socketFD, Connection connection) {
    char buf[BUFFER_SIZE] = "\0";

    ssize_t r = read(connectionFD, buf, BUFFER_SIZE);

    if (!r) {
//        printf("WARNING: receiveFIFO closed\n");
    } else {
        string cmd = string(buf);
        if (cmd.find_first_not_of(' ') != std::string::npos) {
            // There's a non-space.
            printf("DEBUG: valid switch connection POLLIN event: %s\n", connection.getReceiveFIFOName().c_str());
            printf("DEBUG: obtained raw: %s\n", cmd.c_str());
            Packet packet = Packet(cmd);
            printf("DEBUG: Parsed packet: %s\n", packet.toString().c_str());
            if (packet.getType() == ACK) {
                respondACKPacket();
            } else if (packet.getType() == ADD) {
                respondADDPacket(packet.getMessage());
            } else if (packet.getType() == RELAY) {
                respondRELAYPacket(socketFD, packet.getMessage());
            } else {
                // Switch has no other special behavior for other packets
                if (packet.getType() == OPEN) {
                    rOpenCount++;
                } else if (packet.getType() == QUERY) {
                    rQueryCount++;
                }
                printf("ERROR: unexpected %s packet received: %s\n", packet.getType().c_str(), cmd.c_str());
            }
        }
    }
}


/**
 * Check the TCP socket connection to the controller.
 *
 * @param socketFD {@code int} file descriptor to the client TCP socket connected to the server controller.
 * @param tmpbuf  {@code char *} temporary buffer to store obtained messages.
 */
void Switch::check_sock(int socketFD, char *tmpbuf) {
    string msg = get_message(socketFD, tmpbuf);

    Packet packet = Packet(msg);
    if (errno == EINVAL) {
        errno = 0;
    } else {
        if (packet.getType() == ACK) {
            respondACKPacket();
        } else if (packet.getType() == ADD) {
            respondADDPacket(packet.getMessage());
        } else {
            // Switch has no other special behavior for other packets
            if (packet.getType() == OPEN) {
                rOpenCount++;
            } else if (packet.getType() == RELAY) {
                rRelayCount++;
            } else if (packet.getType() == QUERY) {
                rQueryCount++;
            }
            printf("ERROR: unexpected %s packet received: %s\n", packet.getType().c_str(), msg.c_str());
        }
    }
}

/**
 * Parse a line within the Switche's TrafficFile.
 *
 * @param socketFD {@code int}
 * @param line {@code std::string}
 * @return {@code std::string}
 */
string &Switch::parseTrafficFileLine(int socketFD, string &line) {
    int trafficFileLineType = getTrafficFileLineType(line);

    if (trafficFileLineType == INVALID_LINE) {
        return line;
    } else if (trafficFileLineType == DELAY_LINE) {
        trafficFileDelayItem delayItem = parseTrafficDelayItem(line);
        SwitchID tfSwitchID = get<0>(delayItem);
        if (tfSwitchID.getSwitchIDNum() != getSwitchID().getSwitchIDNum()) {
            printf("DEBUG: ignoring line specifying another switch\n");
        } else {
            setDelay(get<2>(delayItem));
        }
        // TODO: act on delay item
    } else if (trafficFileLineType == ROUTE_LINE) {
        trafficFileRouteItem routeItem = parseTrafficRouteItem(line);
        SwitchID tfSwitchID = get<0>(routeItem);
        if (tfSwitchID.getSwitchIDNum() != getSwitchID().getSwitchIDNum()) {
            printf("DEBUG: ignoring line specifying another switch\n");
        } else {
            uint srcIP = get<1>(routeItem);
            uint dstIP = get<2>(routeItem);
            if (resolvePacket(srcIP, dstIP) <= 0) { // did not find rule
                sendQUERYPacket(socketFD, srcIP, dstIP);
            }
        }
    }
    return line;
}

/**
 * The program writes all entries in the flow table, and for each transmitted or received
 * packet type, the program writes an aggregate count of handled packets of this type
 */
void Switch::list() {
    listSwitchStats();
    listPacketStats();
}

/**
 * Attempt to get the index to a matching FlowEntry rule within the Switch's FlowTable for a given traffic packet item.
 *
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 * @return {@code int} index of the matching FlowEntry within the FlowTable.
 *         Else return -1 is no matching FlowEntry was found.
 */
int Switch::getFlowEntryIndex(uint srcIP, uint dstIP) {
    // iterate through flowTable rules
    printf("DEBUG: searching for FlowEntry for: srcIP: %u dstIP: %u\n", srcIP, dstIP);
    int flowTableIndex = 0;
    for (auto const &flowEntry: flowTable) {
        // ensure valid src
        if (srcIP >= flowEntry.srcIPLow && srcIP <= flowEntry.srcIPHigh) {
            // ensure valid dst
            if (dstIP >= flowEntry.dstIPLow && dstIP <= flowEntry.dstIPHigh) {
                string actionName;
                if (flowEntry.actionType == DELIVER) {
                    actionName = "DELIVER";
                } else if (flowEntry.actionType == FORWARD) {
                    actionName = "FORWARD";
                } else if (flowEntry.actionType == DROP) {
                    actionName = "DROP";
                }
                printf("DEBUG: found matching FlowEntry: (srcIP= %u-%u dstIP %u-%u action=%s:%u pri= %u pktCount= %u)\n",
                       flowEntry.srcIPLow, flowEntry.srcIPHigh, flowEntry.dstIPLow, flowEntry.dstIPHigh,
                       actionName.c_str(), flowEntry.actionVal, flowEntry.pri, flowEntry.pktCount);
                return flowTableIndex;
            }
        }
        flowTableIndex++;
    }
    return -1;
}

/**
 * Send a OPEN packet describing the Switch through the given connection.
 *
 * Note: this should only be sent to the controller which communicates through TCP sockets and not FIFOs.
 *
 * @param socketFD {@code int}
 */
void Switch::sendOPENPacket(int socketFD) {
    Message openMessage;
    openMessage.emplace_back(make_tuple("switchID", getSwitchID().getSwitchIDString()));
    openMessage.emplace_back(make_tuple("leftSwitchID", getLeftSwitchID().getSwitchIDString()));
    openMessage.emplace_back(make_tuple("rightSwitchID", getRightSwitchID().getSwitchIDString()));
    openMessage.emplace_back(make_tuple("IPLow", to_string(getIPLow())));
    openMessage.emplace_back(make_tuple("IPHigh", to_string(getIPHigh())));
    openMessage.emplace_back(make_tuple("Address", getServerAddr().getSymbolicName()));
    openMessage.emplace_back(make_tuple("Port", to_string(getPort().getPortNum())));
    Packet openPacket = Packet(OPEN, openMessage);
    // TODO: fix log statement
    printf("INFO: (src= %s, dest= cont) sending OPEN packet: packet: %s\n",
           getSwitchID().getSwitchIDString().c_str(),
           openPacket.toString().c_str());
    send(socketFD, openPacket.toString().c_str(), strlen(openPacket.toString().c_str())+1, 0);
    tOpenCount++;
}

/**
 * Send a QUERY packet describing the unknown packet header through the given connection.
 *
 * @param socketFD {@code int}
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 */
void Switch::sendQUERYPacket(int socketFD, uint srcIP, uint dstIP) {
    Message queryMessage;
    queryMessage.emplace_back(MessageArg("switchID", getSwitchID().getSwitchIDString()));
    queryMessage.emplace_back(MessageArg("srcIP", to_string(srcIP)));
    queryMessage.emplace_back(MessageArg("dstIP", to_string(dstIP)));
    Packet queryPacket = Packet(QUERY, queryMessage);
    // TODO: update log statement
    printf("INFO: (src= %s, dest= cont) sending QUERY packet: packet: %s\n",
           getSwitchID().getSwitchIDString().c_str(),
           queryPacket.toString().c_str());
    send(socketFD, queryPacket.toString().c_str(), strlen(queryPacket.toString().c_str())+1, 0);
    // set packet whether from the traffic file or from a relay into the unsolvedPackets vector
    // to be solved later
    unsolvedPackets.emplace_back(Packet(RELAY, queryMessage));
    tQueryCount++;
}

/**
 * Send a RELAY packet out to the specified connection.
 *
 * @param connection {@code Connection}
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 */
void Switch::sendRELAYPacket(Connection connection, uint srcIP, uint dstIP, SwitchID dstSwitchID) {
    Message relayMessage;
    relayMessage.emplace_back(make_tuple("switchID", getSwitchID().getSwitchIDString()));
    relayMessage.emplace_back(make_tuple("srcIP", to_string(srcIP)));
    relayMessage.emplace_back(make_tuple("dstIP", to_string(dstIP)));
    Packet relayPacket = Packet(RELAY, relayMessage);
    printf("INFO: (src= %s, dest= %s) sending RELAY packet: connection: %s packet: %s\n",
           getSwitchID().getSwitchIDString().c_str(),

           connection.getSendFIFOName().c_str(), relayPacket.toString().c_str());
    write(connection.openSendFIFO(), relayPacket.toString().c_str(),
          strlen(relayPacket.toString().c_str()));
    if (errno == EBADF) { // bad file likely due to other switch being down
        printf("ERROR: failed sending RELAY packet: %s\n", relayPacket.toString().c_str());
        errno = 0;
    }
    tRelayCount++;
}

/**
 * Respond to a ACK packet.
 */
void Switch::respondACKPacket() {
    rAckCount++;
    printf("INFO: ACK packet received:\n");
}

/**
 * Respond to a ADD packet.
 *
 * The switch then stores and applies the received rule within the ADD packet.
 *
 * @param message {@code Message}
 */
void Switch::respondADDPacket(Message message) {
    rAddCount++;
    uint srcIP_lo = static_cast<uint>(stoi(get<1>(message[0])));
    uint srcIP_hi = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP_lo = static_cast<uint>(stoi(get<1>(message[2])));
    uint dstIP_hi = static_cast<uint>(stoi(get<1>(message[3])));
    uint actionType = static_cast<uint>(stoi(get<1>(message[4])));
    uint actionVal = static_cast<uint>(stoi(get<1>(message[5])));
    uint pri = static_cast<uint>(stoi(get<1>(message[6])));
    uint pktCount = static_cast<uint>(stoi(get<1>(message[7])));

    printf("INFO: parsed ADD packet:\n"
           "\tAdding new flowTable rule:\n"
           "\t\tsrcIP_lo: %u srcIPHigh: %u dstIPLow: %u dstIPHigh: %u actionType: %s actionVal: %u pri: %u pktCount: %u\n",
           srcIP_lo, srcIP_hi, dstIP_lo, dstIP_hi, toActionName(actionType).c_str(), actionVal, pri, pktCount);

    FlowEntry newRule = {
            .srcIPLow   = srcIP_lo,
            .srcIPHigh   = srcIP_hi,
            .dstIPLow   = dstIP_lo,
            .dstIPHigh   = dstIP_hi,
            .actionType = actionType,
            .actionVal  = actionVal,
            .pri        = pri,
            .pktCount   = pktCount
    };
    flowTable.emplace_back(newRule);

    // sort and dedupe the flowTable
    sort(flowTable.begin(), flowTable.end());
    flowTable.erase(unique(flowTable.begin(), flowTable.end()), flowTable.end());
    resolveUnsolvedPackets();
}

/**
 * Attempt to resolve any unsolved {@code packet}s.
 *
 * Note: this should be done after responding to a ADD packet.
 */
void Switch::resolveUnsolvedPackets() {
    for (auto iter = unsolvedPackets.begin(); iter != unsolvedPackets.end();) {
        Message message = iter->getMessage();
        uint srcIP = static_cast<uint>(stoi(get<1>(message[1])));
        uint dstIP = static_cast<uint>(stoi(get<1>(message[2])));
        if (resolvePacket(srcIP, dstIP) >= 1) {
            iter = unsolvedPackets.erase(iter);
        } else {
            ++iter;
        }
    }
}

/**
 * Respond to a RELAY packet.
 *
 * A switch may forward a received packet header to a neighbour (as instructed by a
 * matching rule in the flow table). This information is passed to the neighbour in a
 * RELAY packet.
 *
 * @param socketFD {@code int}
 * @param message {@code Message}
 */
void Switch::respondRELAYPacket(int socketFD, Message message) {
    rRelayCount++;
    SwitchID rSwitchID = SwitchID(get<1>(message[0]));
    uint srcIP = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP = static_cast<uint>(stoi(get<1>(message[2])));

    printf("INFO: parsed RELAY packet:\n"
           "\tswitchID: %s srcIP: %u dstIP: %u",
           rSwitchID.getSwitchIDString().c_str(), srcIP, dstIP);

    if (resolvePacket(srcIP, dstIP) <= 0) { // did not find rule
        sendQUERYPacket(socketFD, srcIP, dstIP);
    }
}

/**
 * Resolve a packet's routing via lookup in the {@code FlowTable}.
 *
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 * @return {@code 1} if the relay packet was accepted (DROP, FORWARD, DELIVER), otherwise return {@code 0}.
 */
int Switch::resolvePacket(uint srcIP, uint dstIP) {
    printf("INFO: resolving packet:\n"
           "\tsrcIP: %u dstIP: %u", srcIP, dstIP);
    int fi = getFlowEntryIndex(srcIP, dstIP);
    if (fi >= 0) { // found rule
        FlowEntry flowEntry = flowTable.at(static_cast<unsigned long>(fi));
        // we now have a valid rule that ap
        if (flowEntry.actionType == DELIVER) {
            // this packet is ours
            admitCount++;
        } else if (flowEntry.actionType == FORWARD) {
            if (flowEntry.actionVal == PORT_1) {
                // left switch port 1
                sendRELAYPacket(connections[PORT_1 - 1], srcIP, dstIP, getLeftSwitchID());

            } else if (flowEntry.actionVal == PORT_2) {
                // right switch port 2
                sendRELAYPacket(connections[PORT_2 - 1], srcIP, dstIP, getRightSwitchID());
            } else if (flowEntry.actionVal == PORT_3) {  // FORWARD:3 == DELIVER
                // this packet is ours
                admitCount++;
            } else {
                printf("ERROR: given FORWARD to unsupported port: %u\n", flowEntry.actionVal);
            }
        } else if (flowEntry.actionType == DROP) {
            // do nothing on drop
        }
        flowEntry.pktCount++;
        flowTable[fi] = flowEntry;
        return 1;
    } else {
        return 0;
    }
}

/**
 * Getter for the {@code Switch}'s {@code serverAddr} the IP address of the server's host.
 *
 * Can be either a symbolic name (e.g. util.cs.ualberta.ca, or localhost)
 * or in dotted-decimal format (e.g. 127.0.0.1).
 *
 * @return {@code Address}
 */
Address Switch::getServerAddr() {
    return address;
}

/**
 * Print {@code Switch} specific statistics.
 */
void Switch::listSwitchStats() {
    uint counter = 0;
    printf("%s FlowTable:\n", getSwitchID().getSwitchIDString().c_str());
    for (auto const &flowEntry: flowTable) {
        printf("[%u] (srcIP= %u-%u dstIP %u-%u action=%s:%u pri= %u pktCount= %u)\n",
               counter,
               flowEntry.srcIPLow, flowEntry.srcIPHigh, flowEntry.dstIPLow, flowEntry.dstIPHigh,
               toActionName(flowEntry.actionType).c_str(), flowEntry.actionVal, flowEntry.pri, flowEntry.pktCount);
        counter++;
    }
}

/**
 * Check if the current clock time is greater than or equal to the current delay's {@code endTime}.
 *
 * @return {@code bool}
 */
bool Switch::delayPassed() {
    milliseconds curTime = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );
    return curTime >= endTime;
}

/**
 * Set the current delay's {@code endTime}. If a delay is already inplace and is still active
 * (i.e delayPassed returns {@code false}) add onto the delay's {@code endtime}.
 *  Otherwise set the delay's {@code endTime} to the given interval plus the current clock time.
 *
 * @param interval {@code clock_t}
 */
void Switch::setDelay(milliseconds interval) {
    milliseconds currentTime = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );
    if (!delayPassed()) {
        endTime = interval + endTime;
    } else {
        endTime = interval + currentTime;
    }
    printf("DEBUG: setting delay interval: currentTime: %lims endTime: %lims\n", currentTime, endTime);
}


/**
 * Getter for the {@code Switch}'s {@code SwitchID}.
 *
 * @return {@code SwitchID}
 */
SwitchID Switch::getSwitchID() const {
    return switchID;
}
