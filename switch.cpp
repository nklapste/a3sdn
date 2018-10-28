/**
 * switch.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <sys/types.h>
#include <cstdio>
#include <cstdlib>

#include "switch.h"
#include "controller.h"
#include "packet.h"
#include <sys/signalfd.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <string.h>

#include <assert.h>

/*FIFO stuff*/
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <string>
#include <fcntl.h>
#include <regex>
#include <fstream>
#include <unistd.h>
#include <poll.h>

using namespace std;

// TODO: forward spec and DELIVER SPEC

/**
 * Parse the switch id. Should match the format {@code 'swi'} where {@code 'i'} is a numeric.
 * @param switchID
 * @return
 */
uint parseSwitchID(const string &switchID) {
    regex rgx("(sw)([1-9]+[0-9]*)");
    match_results<string::const_iterator> matches;

    std::regex_match(switchID, matches, rgx);
    for (std::size_t index = 1; index < matches.size(); ++index) {
    }

    if (std::regex_search(switchID, matches, rgx)) {
        return static_cast<uint>(std::stoi(matches[2], nullptr, 10));
    } else {
        // we failed to parse the switch id
        printf("ERROR: invalid switch id argument: %s\n", switchID.c_str());
        exit(1);
    }
}

/**
 * The program writes all entries in the flow table, and for each transmitted or received
 * packet type, the program writes an aggregate count of handled packets of this type
 */
void Switch::list() {
    uint counter = 0;
    printf("sw%u Flow table:\n", switchID);
    for (auto const &flowEntry: flowTable) {
        string actionName;
        if (flowEntry.actionType == 0) {
            actionName = "DELIVER";
        } else if (flowEntry.actionType == 1) {
            actionName = "FORWARD";
        } else if (flowEntry.actionType == 2) {
            actionName = "DROP";
        }
        printf("[%u] (srcIP= %u-%u dstIP %u-%u action=%s:%u pri= %u pktCount= %u)\n",
               counter,
               flowEntry.srcIP_lo, flowEntry.srcIP_hi, flowEntry.dstIP_lo, flowEntry.dstIP_hi,
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
 * Initialize a switch.
 *
 * @param switchID
 * @param leftSwitchID
 * @param rightSwitchID
 * @param trafficFile
 * @param IPLow
 * @param IPHigh
 */
Switch::Switch(string &switchID, string &leftSwitchID, string &rightSwitchID, string &trafficFile, uint IPLow,
               uint IPHigh) {
    printf("DEBUG: Creating switch: %s trafficFile: %s swj: %s swk: %s IPLow: %u IPHigh: %u\n",
           switchID.c_str(), trafficFile.c_str(), leftSwitchID.c_str(), rightSwitchID.c_str(), IPLow, IPHigh);
    /*
     *   [srcIP lo= 0, srcIP hi= MAXIP, destIP lo= IPlow, destIP hi= IPhigh,
     *   actionType= FORWARD, actionVal= 3, pri= MINPRI, pktCount= 0]
     */
    FlowEntry init_rule = {
            .srcIP_lo = 0,
            .srcIP_hi = MAX_IP,
            .dstIP_lo = IPLow,
            .dstIP_hi = IPHigh,
            .actionType= DELIVER,
            .actionVal=3,
            .pri=MIN_PRI,
            .pktCount=0
    };

    Switch::IPHigh = IPHigh;
    Switch::IPLow = IPLow;
    flowTable.push_back(init_rule);

    Switch::trafficFile = trafficFile;

    // create Connection to controller
    Switch::switchID = parseSwitchID(switchID);
    connections.emplace_back(Connection(Switch::switchID, CONTROLLER_ID));

    neighbors = 0;
    // create Connection to the left switch
    // can potentially be a nullptr
    if (leftSwitchID != NULL_ID) {
        neighbors++;
        Switch::leftSwitchID = parseSwitchID(leftSwitchID);

    } else {
        Switch::leftSwitchID = -1;
    }
    connections.emplace_back(Connection(Switch::switchID, Switch::leftSwitchID));
    // create Connection to the right switch
    // can potentially be a nullptr
    if (rightSwitchID != NULL_ID) {
        neighbors++;
        Switch::rightSwitchID = parseSwitchID(rightSwitchID);
    } else {
        Switch::rightSwitchID = -1;
    }
    connections.emplace_back(Connection(Switch::switchID, Switch::rightSwitchID));

    printf("INFO: I am switch: %i\n", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}

/**
 * Parse a line in a trafficFile into a {@code trafficItem}.
 *
 * @param trafficFileLine {@code std::string}
 * @return {@code trafficItem}
 */
trafficFileItem parseTrafficItem(string &trafficFileLine) {
    istringstream iss(trafficFileLine);
    vector<string> trafficFileItems((istream_iterator<string>(iss)),
                                    istream_iterator<string>());
    uint switchID = parseSwitchID(trafficFileItems.at(0));
    uint srcIP = static_cast<uint>(stoi(trafficFileItems.at(1)));
    uint dstIP = static_cast<uint>(stoi(trafficFileItems.at(2)));
    printf("DEBUG: Parsed trafficFileItem: switchID: %u srcIP: %u dst: %u\n",
           switchID, srcIP, dstIP);
    return make_tuple(switchID, srcIP, dstIP);
}

/**
 * Start the {@code Switch} loop.
 */
void Switch::start() {
    struct pollfd pfds[connections.size() + 3];
    char buf[1024];

    // get fd for stdin
    pfds[0].fd = STDIN_FILENO;
    string line;
    ifstream trafficFileStream(trafficFile);

    // get the fd for every receiving FIFO
    for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
        pfds[i].fd = connections[i - 1].openReceiveFIFO();
    }

    // When a switch starts, it sends an OPEN packet to the controller.
    // The carried message contains the switch number, the numbers of its neighbouring switches (if any),
    // and the range of IP addresses served by the switch.
    sendOPENPacket(connections[0]);

    // TODO: wait for ack?
    int err;
    sigset_t sigset;
    /* Create a sigset of all the signals that we're interested in */
    err = sigemptyset(&sigset);
    assert(err == 0);
    err = sigaddset(&sigset, SIGUSR1);
    assert(err == 0);
    /* We must block the signals in order for signalfd to receive them */
    err = sigprocmask(SIG_BLOCK, &sigset, nullptr);
    assert(err == 0);
    /* This is the main loop */
    pfds[connections.size()+2].fd = signalfd(-1, &sigset, 0);;
    pfds[connections.size()+2].events = POLLIN;

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
        pfds[0].events = POLLIN;
        pfds[connections.size()+2].events = POLLIN;
        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
            pfds[i].events = POLLIN;
        }

        poll(pfds, connections.size() + 3, 0);
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
                printf("INFO: exit command received: terminating\n");
                exit(0);
            } else {
                printf("ERROR: invalid Controller command: %s\n"
                       "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
            }
            fflush(stdout);
            fflush(stdin);
        }

        /*
         * 3. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         */
        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
            if (pfds[i].revents & POLLIN) {
                printf("DEBUG: pfds[%lu] has connection POLLIN event: %s\n", i,
                       connections[i - 1].getReceiveFIFOName().c_str());
                int r = read(pfds[i].fd, buf, 1024);
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
        if(pfds[connections.size()+2].revents & POLLIN){
            // TODO works but blocks stuff
            struct signalfd_siginfo info;
            /* We have a valid signal, read the info from the fd */
            int r = read(pfds[connections.size()+2].fd, &info, sizeof(info));
            if(!r){
                printf("WARNING: signal reading error\n");
            }
            unsigned sig = info.ssi_signo;
            if (sig == SIGUSR1) {
                printf("DEBUG: received SIGUSR1\n");
                list();
            }
        }
    }
}

/**
 * Send a OPEN packet describing the swtich through the given connection.
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
    write(connection.openSendFIFO(), openPacket.toString().c_str(), strlen(openPacket.toString().c_str()));
    tOpenCount++;
}

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
        trafficFileItem tfItem = parseTrafficItem(line);
        uint tfSwitchID = get<0>(tfItem);
        uint srcIP = get<0>(tfItem);
        uint dstIP = get<0>(tfItem);

        int fi = getFlowEntry(switchID, srcIP, dstIP);
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
                    printf("ERROR: given FORWARD to unknown port: %u", flowEntry.actionVal);
                }
            } else if (flowEntry.actionType == DROP) {
                // do nothing
            }
            flowEntry.pktCount++;
            flowTable[fi] = flowEntry;
        } else if (fi < 0) { // did not find rule
            sendQUERYPacket(connections[0], srcIP, dstIP);
            // TODO: wait for response and possibly reply?
        }
    }
    return line;
}

/**
 * Constructor by the controller to make a switch for upkeep.
 *
 * Note: this constructor not intended for actual usage by invoking the {@code start()} method.
 *
 * @param switchID
 * @param neighbors
 * @param IPLow
 * @param IPHigh
 */
Switch::Switch(uint switchID, int leftSwitchID, int rightSwitchID, uint IPLow, uint IPHigh) :
        switchID(switchID), leftSwitchID(leftSwitchID), rightSwitchID(rightSwitchID),
        IPLow(IPLow), IPHigh(IPHigh) {
    if (leftSwitchID > 0) {
        neighbors++;
    }
    if (rightSwitchID > 0) {
        neighbors++;
    }
}

/**
 * Getter for a switch's {@code switchID}.
 *
 * @return {@code uint}
 */
uint Switch::getID() {
    return switchID;
}

/**
 * Getter for a switch's {@code IPHigh}.
 *
 * @return {@code uint}
 */
uint Switch::getIPHigh() {
    return IPHigh;
}

/**
 * Getter for a switch's {@code IPLow}.
 *
 * @return {@code uint}
 */
uint Switch::getIPLow() {
    return IPLow;
}

/**
 * Getter for a switch's {@code rightSwitchID}
 *
 * Note: if the switch does not have a right neighboring switch {@code -1} will be returned.
 *
 * @return {@code int}
 */
int Switch::getRightSwitchID() {
    return rightSwitchID;
}

/**
 * Getter for a switch's {@code leftSwitchID}
 *
 * Note: if the switch does not have a left neighboring switch {@code -1} will be returned.
 *
 * @return {@code int}
 */
int Switch::getLeftSwitchID() {
    return leftSwitchID;
}

/**
 * Attempt to get a rule for a specific traffic packet item.
 *
 * @param switchID
 * @param srcIP
 * @param dstIP
 * @return {@code FlowEntry}
 */
int Switch::getFlowEntry(uint switchID, uint srcIP, uint dstIP) {
    // iterate through flowTable rules
    int index=0;
    for (auto const &flowEntry: flowTable) {
        // ensure valid src
        if (srcIP >= flowEntry.srcIP_lo || srcIP <= flowEntry.srcIP_hi) {
            // ensure valid dst
            if (dstIP >= flowEntry.dstIP_lo || dstIP <= flowEntry.dstIP_hi) {
                string actionName;
                if (flowEntry.actionType == DELIVER) {
                    actionName = "DELIVER";
                } else if (flowEntry.actionType == FORWARD) {
                    actionName = "FORWARD";
                } else if (flowEntry.actionType == DROP) {
                    actionName = "DROP";
                }
                printf("found matching flowtable rule: (srcIP= %u-%u dstIP %u-%u action=%s:%u pri= %u pktCount= %u)\n",
                       flowEntry.srcIP_lo, flowEntry.srcIP_hi, flowEntry.dstIP_lo, flowEntry.dstIP_hi,
                       actionName.c_str(), flowEntry.actionVal, flowEntry.pri, flowEntry.pktCount);
                return index;
            }
        }
        index++;
    }
    return -1;
}

/**
 * Respond to a ADD packet.
 *
 * A switch may forward a received packet header to a neighbour (as instructed by a
 * matching rule in the flow table).  This information is passed to the neighbour in a
 * RELAY packet.
 *
 * @param message
 */
void Switch::respondRELAYPacket(Message message) {
    rRelayCount++;
    uint rSwitchID = static_cast<uint>(stoi(get<1>(message[0])));
    uint srcIP = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP = static_cast<uint>(stoi(get<1>(message[2])));

    // TODO figure out way to update flow entry
    int fi = getFlowEntry(rSwitchID, srcIP, dstIP);
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
                printf("ERROR: given FORWARD to sw%u this does not match any neighbors",
                       flowEntry.actionVal);
            }
        } else if (flowEntry.actionType == DROP) {

        }
        flowEntry.pktCount++;
        flowTable[fi]=flowEntry;
    } else if (fi < 0) { // did not find rule
        sendQUERYPacket(connections[PORT_0], srcIP, dstIP);
        // TODO: wait for response and possibly reply?
    }
}

/**
 * Send a QUERY packet describing the unknown packet header through the given connection.
 *
 * @param connection {@code uint}
 * @param srcIP {@code uint}
 * @param dstIP {@code uint}
 */
void Switch::sendQUERYPacket(Connection connection, uint srcIP, uint dstIP) {
    Message queryMessage;
    queryMessage.emplace_back(MessageArg("switchID", to_string(switchID)));
    queryMessage.emplace_back(MessageArg("srcIP", to_string(srcIP)));
    queryMessage.emplace_back(MessageArg("dstIP", to_string(dstIP)));
    Packet queryPacket = Packet(QUERY, queryMessage);
    write(connection.openSendFIFO(), queryPacket.toString().c_str(),
          strlen(queryPacket.toString().c_str()));
    tQueryCount++;
}

/**
 * Respond to a ACK packet.
 */
void Switch::respondACKPacket() {
    rAckCount++;
    printf("ACK packet received:\n");
}


/**
 * Respond to a ADD packet.
 *
 * The switch then stores and applies the received rule within the ADD packet.
 *
 * @param message
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

    printf("Parse ADD packet: Adding new flowTable rule:\n"
           "\tsrcIP_lo: %u srcIP_hi: %u dstIP_lo: %u dstIP_hi: %u actionType: %u actionVal: %u pri: %u pktCount: %u\n",
           srcIP_lo, srcIP_hi, dstIP_lo, dstIP_hi, actionType, actionVal, pri, pktCount);

    FlowEntry newRule = {
            .srcIP_lo   = srcIP_lo,
            .srcIP_hi   = srcIP_hi,
            .dstIP_lo   = dstIP_lo,
            .dstIP_hi   = dstIP_hi,
            .actionType = actionType,
            .actionVal  = actionVal,
            .pri        = pri,
            .pktCount   = pktCount
    };
    flowTable.emplace_back(newRule);
}

/**
 *
 */
void Switch::sendRELAYPacket(Connection connection, uint srcIP, uint dstIP) {
    Message relayMessage;
    relayMessage.emplace_back(make_tuple("switchID", to_string(switchID)));
    relayMessage.emplace_back(make_tuple("srcIP", to_string(srcIP)));
    relayMessage.emplace_back(make_tuple("dstIP", to_string(dstIP)));
    Packet relayPacket = Packet(RELAY, relayMessage);
    write(connection.openSendFIFO(), relayPacket.toString().c_str(),
          strlen(relayPacket.toString().c_str()));
    tRelayCount++;
}

