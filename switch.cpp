/**
 * switch.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

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

/* FIFO stuff */
#include <poll.h>

#include "controller.h"
#include "switch.h"

#define LIST_CMD "list"
#define EXIT_CMD "exit"

#define BUFFER_SIZE 1024

#define PDFS_SIZE 5
#define PDFS_STDIN 0
#define PDFS_CONTROLLER 1
#define PDFS_LEFT_SWITCH 2
#define PDFS_RIGHT_SWITCH 3
#define PDFS_SIGNAL 4

using namespace std;

/**
 * Initialize a switch.
 *
 * @param switchID {@code std::string}
 * @param leftSwitchID {@code std::string}
 * @param rightSwitchID {@code std::string}
 * @param trafficFile {@code std::string}
 * @param IPRangeStr {@code std::string}
 */
Switch::Switch(string &switchID, string &leftSwitchID, string &rightSwitchID, string &trafficFile, string &IPRangeStr) {
    IPRange ipRange = parseIPRange(IPRangeStr);
    IPLow = get<0>(ipRange);
    IPHigh = get<1>(ipRange);

    // create and add the Switch's initial FLowEntry rule
    FlowEntry init_rule = {
            .srcIPLow   = MIN_IP,
            .srcIPHigh   = MAX_IP,
            .dstIPLow   = IPLow,
            .dstIPHigh   = IPHigh,
            .actionType = DELIVER,
            .actionVal  = PORT_3,
            .pri        = MIN_PRI,
            .pktCount   = 0
    };

    flowTable.push_back(init_rule);

    Switch::trafficFile = trafficFile;

    // create Connection to controller
    Switch::switchID = parseSwitchID(switchID);
    connections.emplace_back(Connection(Switch::switchID, CONTROLLER_ID));


    // create Connection to the left switch
    // can potentially be a nullptr
    if (leftSwitchID != NULL_SWITCH_FLAG) {
        Switch::leftSwitchID = parseSwitchID(leftSwitchID);
        connections.emplace_back(Connection(Switch::switchID, static_cast<uint>(Switch::leftSwitchID)));

    } else {
        Switch::leftSwitchID = NULL_SWITCH_ID;
        connections.emplace_back(Connection());

    }

    // create Connection to the right switch
    // can potentially be a nullptr
    if (rightSwitchID != NULL_SWITCH_FLAG) {
        Switch::rightSwitchID = parseSwitchID(rightSwitchID);
        connections.emplace_back(Connection(Switch::switchID, static_cast<uint>(Switch::rightSwitchID)));
    } else {
        Switch::rightSwitchID = NULL_SWITCH_ID;
        connections.emplace_back(Connection());
    }
    printf("INFO: created switch: %s trafficFile: %s swj: %s swk: %s IPLow: %u IPHigh: %u\n",
           switchID.c_str(), trafficFile.c_str(), leftSwitchID.c_str(), rightSwitchID.c_str(), IPLow, IPHigh);
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
Switch::Switch(uint switchID, int leftSwitchID, int rightSwitchID, uint IPLow, uint IPHigh) :
        switchID(switchID), leftSwitchID(leftSwitchID), rightSwitchID(rightSwitchID),
        IPLow(IPLow), IPHigh(IPHigh) {
}

/**
 * Getter for a switch's {@code switchID}.
 *
 * @return {@code uint}
 */
uint Switch::getID() const {
    return switchID;
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
 * @return {@code int}
 */
int Switch::getLeftSwitchID() const {
    return leftSwitchID;
}

/**
 * Getter for a switch's {@code rightSwitchID}.
 *
 * Note: if the switch does not have a right neighboring switch {@code -1} will be returned.
 *
 * @return {@code int}
 */
int Switch::getRightSwitchID() const {
    return rightSwitchID;
}

/**
 * Start the {@code Switch} loop.
 */
void Switch::start() {
    struct pollfd pfds[PDFS_SIZE];
    char buf[BUFFER_SIZE];

    // setup file descriptions or stdin and all connection FIFOs
    pfds[PDFS_STDIN].fd = STDIN_FILENO;
    pfds[PDFS_CONTROLLER].fd = connections[0].openReceiveFIFO();
    if (leftSwitchID >= 1) {
        pfds[PDFS_LEFT_SWITCH].fd = connections[1].openReceiveFIFO();
    }
    if (rightSwitchID >= 1) {
        pfds[PDFS_RIGHT_SWITCH].fd = connections[2].openReceiveFIFO();
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

    string line;
    ifstream trafficFileStream(trafficFile);

    // When a switch starts, it sends an OPEN packet to the controller.
    // The carried message contains the switch number, the numbers of its neighbouring switches (if any),
    // and the range of IP addresses served by the switch.
    sendOPENPacket(connections[0]);
    // TODO: wait for ack?

    // enter the switch loop
    for (;;) {
        /*
         * 1.  Read and process a single line from the traffic line (if the EOF has not been reached yet). The
         *     switch ignores empty lines, comment lines, and lines specifying other handling switches. A
         *     packet header is considered admitted if the line specifies the current switch
         *
         *     Note: After reading all lines in the traffic file, the program continues to monitor and process
         *     keyboard commands, and the incoming packets from neighbouring devices.
         */
        if (trafficFileStream.is_open()) {
            if (getline(trafficFileStream, line)) {
                parseTrafficFileLine(line);
            } else {
                trafficFileStream.close();
                printf("DEBUG: finished reading traffic file\n");
            }
        }

        /*
         * 2. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        for (auto &pfd : pfds) {
            pfd.events = POLLIN;
        }
        int ret = poll(pfds, PDFS_SIZE, 20);
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
         * 3. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         */

        for (std::vector<Connection>::size_type i = 1; i < 4; i++) {
            if (i == 2 && leftSwitchID < MIN_SWITCHES) {
                continue;
            }
            if (i == 3 && rightSwitchID < MIN_SWITCHES) {
                continue;
            }
            if (pfds[i].revents & POLLIN) {
                printf("DEBUG: pfds[%lu] has connection POLLIN event: %s\n", i,
                       connections[i - 1].getReceiveFIFOName().c_str());
                ssize_t r = read(pfds[i].fd, buf, BUFFER_SIZE);
                if (!r) {
                    printf("WARNING: receiveFIFO closed\n");
                }
                string cmd = string(buf);
                Packet packet = Packet(cmd);
                printf("DEBUG: Parsed packet: %s\n", packet.toString().c_str());
                if (packet.getType() == ACK) {
                    respondACKPacket();
                } else if (packet.getType() == ADD) {
                    respondADDPacket(packet.getMessage());

                    // after getting a new FlowEntry attempt to resolve any unslovedPackets
                    auto it = unsolvedPackets.begin();
                    while(it != unsolvedPackets.end()) {
                        if(respondRELAYPacket(it->getMessage()) == 1) {
                            // if we respond with a DROP, DELIVER, or FORWARD we have solved the packet
                            it = unsolvedPackets.erase(it);
                        }
                        else ++it;
                    }

                } else if (packet.getType() == RELAY) {
                    respondRELAYPacket(packet.getMessage());
                } else {
                    // Switch has no other special behavior for other packets
                    if (packet.getType() == OPEN) {
                        rOpenCount++;
                    } else if (packet.getType() == RELAY) {
                        rQueryCount++;
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
                printf("DEBUG: received SIGUSR1\n");
                list();
            }
        }
        // clear buffer
        memset(buf, 0, sizeof buf);
    }
}

/**
 * Parse the {@code IPRange} argument. Which follows the format IPLow-IPHigh.
 *
 * @param IPRangeString {@code std::string}
 * @return {@code IPRange} a tuple of (IPLow, IPHigh)
 */
IPRange Switch::parseIPRange(const string &IPRangeString) {
    string delimiter = "-";
    uint IPLow = static_cast<uint>(stoi(IPRangeString.substr(0, IPRangeString.find(delimiter))));
    uint IPHigh = static_cast<uint>(stoi(
            IPRangeString.substr(IPRangeString.find(delimiter) + 1, IPRangeString.size() - 1)));
    if (IPHigh > MAX_IP) {
        printf("ERROR: invalid IPHigh: %u MAX_IP: %u", IPHigh, MAX_IP);
        exit(EINVAL);
    }
    if (IPLow < MIN_IP) {
        printf("ERROR: invalid IPLow: %u MIN_IP: %u", IPLow, MIN_IP);
        exit(EINVAL);
    }
    if (IPLow > IPHigh) {
        printf("ERROR: invalid IP range: IPLow: %u greater than IPHigh: %u", IPLow, IPHigh);
        exit(EINVAL);
    }
    return make_tuple(IPLow, IPHigh);
}

/**
 * Parse the switch id. Should match the format {@code 'swi'} where {@code 'i'} is a numeric character.
 *
 * @param switchID {@code std::string}
 * @return {@code uint}
 */
uint Switch::parseSwitchID(const string &switchID) {
    regex rgx("(sw)([1-9]+[0-9]*)");
    match_results<string::const_iterator> matches;

    std::regex_match(switchID, matches, rgx);
    for (std::size_t index = 1; index < matches.size(); ++index) {
    }

    if (std::regex_search(switchID, matches, rgx)) {
        uint switchID_ = static_cast<uint>(std::stoi(matches[2], nullptr, 10));
        if (switchID_<MIN_SWITCHES){
            printf("ERROR: switchID is to low: %u\n"
                   "\tMIN_SWITCHES=%u\n", switchID_, MIN_SWITCHES);
            exit(EINVAL);
        }else if (switchID_>MAX_SWITCHES){
            printf("ERROR: switchID is to high: %u\n"
                   "\tMAX_SWITCHES=%u\n", switchID_, MAX_SWITCHES);
            exit(EINVAL);
        } else {
            return switchID_;
        }
    } else {
        printf("ERROR: invalid switchID argument: %s\n", switchID.c_str());
        exit(EINVAL);
    }
}

/**
 * Parse a trafficFile item in string format into a {@code trafficItem}.
 *
 * @param trafficFileItem {@code std::string}
 * @return {@code trafficItem}
 */
trafficFileItem Switch::parseTrafficFileItem(string &trafficFileItem) {
    istringstream iss(trafficFileItem);
    vector<string> trafficFileItems((istream_iterator<string>(iss)), istream_iterator<string>());
    uint switchID = parseSwitchID(trafficFileItems.at(0));
    uint srcIP = static_cast<uint>(stoi(trafficFileItems.at(1)));
    uint dstIP = static_cast<uint>(stoi(trafficFileItems.at(2)));
    printf("DEBUG: parsed trafficFileItem: switchID: %u srcIP: %u dst: %u\n",
           switchID, srcIP, dstIP);
    return make_tuple(switchID, srcIP, dstIP);
}

/**
 * Parse a line within the Switche's TrafficFile.
 *
 * @param line {@code std::string}
 * @return {@code std::string}
 */
string &Switch::parseTrafficFileLine(string &line) {
    printf("DEBUG: read traffic file line: %s\n", line.c_str());
    if (line.length() < 1) {
        printf("WARNING: ignoring invalid line: %s\n", line.c_str());
    } else if (line.substr(0, 1) == "#") {
        printf("DEBUG: ignoring comment line\n");
    } else if (line.substr(0, 3) != "sw" + to_string(switchID)) {
        printf("DEBUG: ignoring line specifying another switch\n");
    } else {
        printf("DEBUG: found line specifying self: %s\n", line.c_str());
        trafficFileItem tfItem = parseTrafficFileItem(line);
//        uint tfSwitchID = get<0>(tfItem);
        uint srcIP = get<1>(tfItem);
        uint dstIP = get<2>(tfItem);

        int fi = getFlowEntryIndex(srcIP, dstIP);
        if (fi >= 0) { // found rule
            FlowEntry flowEntry = flowTable.at(fi);
            // we now have a valid rule that applies to the traffic item and the switch
            if (flowEntry.actionType == DELIVER) {
                // this is our packet
                admitCount++;
            } else if (flowEntry.actionType == FORWARD) {
                if (flowEntry.actionVal == PORT_1) {
                    sendRELAYPacket(connections[PORT_1], srcIP, dstIP);
                } else if (flowEntry.actionVal == PORT_2) {
                    sendRELAYPacket(connections[PORT_2], srcIP, dstIP);
                } else {
                    printf("ERROR: given FORWARD to unknown port: %u\n", flowEntry.actionVal);
                }
            } else if (flowEntry.actionType == DROP) {
                // do nothing
            }
            flowEntry.pktCount++;
            flowTable[fi] = flowEntry;
        } else if (fi < 0) { // did not find rule
            sendQUERYPacket(connections[0], srcIP, dstIP);
        }
    }
    return line;
}

/**
 * The program writes all entries in the flow table, and for each transmitted or received
 * packet type, the program writes an aggregate count of handled packets of this type
 */
void Switch::list() {
    uint counter = 0;
    printf("sw%u FlowTable:\n", switchID);
    for (auto const &flowEntry: flowTable) {
        string actionName;
        if (flowEntry.actionType == DELIVER) {
            actionName = "DELIVER";
        } else if (flowEntry.actionType == FORWARD) {
            actionName = "FORWARD";
        } else if (flowEntry.actionType == DROP) {
            actionName = "DROP";
        }
        printf("[%u] (srcIP= %u-%u dstIP %u-%u action=%s:%u pri= %u pktCount= %u)\n",
               counter,
               flowEntry.srcIPLow, flowEntry.srcIPHigh, flowEntry.dstIPLow, flowEntry.dstIPHigh,
               actionName.c_str(), flowEntry.actionVal, flowEntry.pri, flowEntry.pktCount);
        counter++;
    }

    printf("Packet Stats:\n");
    printf("\tReceived:    OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYIN: %u, ADMIT:%u\n",
           rOpenCount, rAckCount, rQueryCount, rAddCount, rRelayCount, admitCount);
    printf("\tTransmitted: OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYOUT:%u\n",
           tOpenCount, tAckCount, tQueryCount, tAddCount, tRelayCount);
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
 * @param connection {@code Connection}
 */
void Switch::sendOPENPacket(Connection connection) {
    Message openMessage;
    openMessage.emplace_back(make_tuple("switchID", to_string(switchID)));
    openMessage.emplace_back(make_tuple("leftSwitchID", to_string(leftSwitchID)));
    openMessage.emplace_back(make_tuple("rightSwitchID", to_string(rightSwitchID)));
    openMessage.emplace_back(make_tuple("IPLow", to_string(IPLow)));
    openMessage.emplace_back(make_tuple("IPHigh", to_string(IPHigh)));
    Packet openPacket = Packet(OPEN, openMessage);
    printf("INFO: sending OPEN packet: connection: %s packet: %s\n",
           connection.getSendFIFOName().c_str(), openPacket.toString().c_str());
    write(connection.openSendFIFO(), openPacket.toString().c_str(), strlen(openPacket.toString().c_str()));
    tOpenCount++;
}

/**
 * Send a QUERY packet describing the unknown packet header through the given connection.
 *
 * @param connection {@code Connection}
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 */
void Switch::sendQUERYPacket(Connection connection, uint srcIP, uint dstIP) {
    Message queryMessage;
    queryMessage.emplace_back(MessageArg("switchID", to_string(switchID)));
    queryMessage.emplace_back(MessageArg("srcIP", to_string(srcIP)));
    queryMessage.emplace_back(MessageArg("dstIP", to_string(dstIP)));
    Packet queryPacket = Packet(QUERY, queryMessage);
    printf("INFO: sending QUERY packet: connection: %s packet: %s\n",
           connection.getSendFIFOName().c_str(), queryPacket.toString().c_str());
    write(connection.openSendFIFO(), queryPacket.toString().c_str(),
          strlen(queryPacket.toString().c_str()));
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
void Switch::sendRELAYPacket(Connection connection, uint srcIP, uint dstIP) {
    Message relayMessage;
    relayMessage.emplace_back(make_tuple("switchID", to_string(switchID)));
    relayMessage.emplace_back(make_tuple("srcIP", to_string(srcIP)));
    relayMessage.emplace_back(make_tuple("dstIP", to_string(dstIP)));
    Packet relayPacket = Packet(RELAY, relayMessage);
    printf("INFO: sending RELAY packet: connection: %s packet: %s\n",
           connection.getSendFIFOName().c_str(), relayPacket.toString().c_str());
    write(connection.openSendFIFO(), relayPacket.toString().c_str(),
          strlen(relayPacket.toString().c_str()));
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
    uint srcIP_lo   = static_cast<uint>(stoi(get<1>(message[0])));
    uint srcIP_hi   = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP_lo   = static_cast<uint>(stoi(get<1>(message[2])));
    uint dstIP_hi   = static_cast<uint>(stoi(get<1>(message[3])));
    uint actionType = static_cast<uint>(stoi(get<1>(message[4])));
    uint actionVal  = static_cast<uint>(stoi(get<1>(message[5])));
    uint pri        = static_cast<uint>(stoi(get<1>(message[6])));
    uint pktCount   = static_cast<uint>(stoi(get<1>(message[7])));

    printf("INFO: parsed ADD packet:\n"
           "\tAdding new flowTable rule:\n"
           "\t\tsrcIP_lo: %u srcIPHigh: %u dstIPLow: %u dstIPHigh: %u actionType: %u actionVal: %u pri: %u pktCount: %u\n",
           srcIP_lo, srcIP_hi, dstIP_lo, dstIP_hi, actionType, actionVal, pri, pktCount);

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
}

/**
 * Respond to a RELAY packet.
 *
 * A switch may forward a received packet header to a neighbour (as instructed by a
 * matching rule in the flow table). This information is passed to the neighbour in a
 * RELAY packet.
 *
 * @param message {@code Message}
 * @return {@code 1} if the relay packet was accepted (DROP, FORWARD, DELIVER), otherwise return {@code 0}.
 */
int Switch::respondRELAYPacket(Message message) {
    rRelayCount++;
    uint rSwitchID = static_cast<uint>(stoi(get<1>(message[0])));
    uint srcIP     = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP     = static_cast<uint>(stoi(get<1>(message[2])));

    printf("INFO: parsed RELAY packet:\n"
           "\tswitchID: %u srcIP: %u dstIP: %u",
           rSwitchID, srcIP, dstIP);

    int fi = getFlowEntryIndex(srcIP, dstIP);
    if (fi >= 0) { // found rule
        FlowEntry flowEntry = flowTable.at(fi);
        // we now have a valid rule that ap
        if (flowEntry.actionType == DELIVER) {
            // this packet is ours
            admitCount++;
        } else if (flowEntry.actionType == FORWARD) {
            if (flowEntry.actionVal == PORT_1) {
                // left switch port 1
                sendRELAYPacket(connections[PORT_1], srcIP, dstIP);

            } else if (flowEntry.actionVal == PORT_2) {
                // right switch port 2
                sendRELAYPacket(connections[PORT_2], srcIP, dstIP);
            } else {
                printf("ERROR: given FORWARD to unsupported port: %u\n", flowEntry.actionVal);
            }
        } else if (flowEntry.actionType == DROP) {
            // do nothing on drop
        }
        flowEntry.pktCount++;
        flowTable[fi] = flowEntry;
        return 1;
    } else if (fi < 0) { // did not find rule
        sendQUERYPacket(connections[PORT_0], srcIP, dstIP);
        return 0;
    }
}