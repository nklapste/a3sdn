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

/**
 * Parse the switch id. Should match the format {@code 'swi'} where {@code 'i'} is a numeric.
 * @param switchId
 * @return
 */
uint parseSwitchId(const string &switchId) {
    regex rgx("(sw)([1-9]+[0-9]*)");
    match_results<string::const_iterator> matches;

    std::regex_match(switchId, matches, rgx);
    for (std::size_t index = 1; index < matches.size(); ++index) {
    }

    if (std::regex_search(switchId, matches, rgx)) {
        return static_cast<uint>(std::stoi(matches[2], nullptr, 10));
    } else {
        // we failed to parse the switch id
        printf("ERROR: invalid switch id argument: %s\n", switchId.c_str());
        exit(1);
    }
}

/**
 * The program writes all entries in the flow table, and for each transmitted or received
 * packet type, the program writes an aggregate count of handled packets of this type
 */
void Switch::list() {
    uint counter = 0;
    printf("sw%u Flow table:\n", switchId);
    for (auto const &flowEntry: flowTable) {
        string actionName;
        if (flowEntry.actionType == 0){
            actionName = "DELIVER";
        } else if (flowEntry.actionType == 1){
            actionName = "FORWARD";
        } else if (flowEntry.actionType == 2){
            actionName = "DROP";
        }
        printf("[%u] (srcIP= %u-%u dstIP %u-%u action=%s:%u pri= %u pktCount= %u)\n",
               counter,
               flowEntry.srcIP_lo, flowEntry.srcIP_hi, flowEntry.dstIP_lo, flowEntry.dstIP_hi,
               actionName.c_str(), flowEntry.actionVal, flowEntry.pri, flowEntry.pktCount);
        counter++;
    }

    printf("Packet Stats:\n");
    printf("\tReceived:    OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYIN: %u\n",
            rOpenCount, rAckCount, rQueryCount, rAddCount, rRelayCount);
    printf("\tTransmitted: OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYOUT:%u\n",
            tOpenCount, tAckCount, tQueryCount, tAddCount, tRelayCount);
}

/**
 * Initialize a switch.
 *
 * @param switchId
 * @param leftSwitchId
 * @param rightSwitchId
 * @param trafficFile
 * @param ipLow
 * @param ipHigh
 */
Switch::Switch(string &switchId, string &leftSwitchId, string &rightSwitchId, string &trafficFile, uint ipLow,
               uint ipHigh) {
    printf("Creating switch: %s trafficFile: %s swj: %s swk: %s ipLow: %u ipHigh: %u\n",
           switchId.c_str(), trafficFile.c_str(), leftSwitchId.c_str(), rightSwitchId.c_str(), ipLow, ipHigh);
    /*
     *   [srcIP lo= 0, srcIP hi= MAXIP, destIP lo= IPlow, destIP hi= IPhigh,
     *   actionType= FORWARD, actionVal= 3, pri= MINPRI, pktCount= 0]
     */
    FlowEntry init_rule = {
            .srcIP_lo = 0,
            .srcIP_hi = MAX_IP,
            .dstIP_lo = ipLow,
            .dstIP_hi = ipHigh,
            .actionType= FORWARD,
            .actionVal=3,
            .pri=MIN_PRI,
            .pktCount=0
    };

    Switch::ipHigh = ipHigh;
    Switch::ipLow = ipLow;
    flowTable.push_back(init_rule);

    Switch::trafficFile = trafficFile;

    // create Connection to controller
    Switch::switchId = parseSwitchId(switchId);
    connections.emplace_back(Connection(Switch::switchId, CONTROLLER_ID));

    neighbors = 0;
    // create Connection to the left switch
    // can potentially be a nullptr
    if (leftSwitchId != NULL_ID) {
        neighbors++;
        Switch::leftSwitchId = parseSwitchId(leftSwitchId);
        connections.emplace_back(Connection(Switch::switchId, Switch::leftSwitchId));
    } else {
        Switch::leftSwitchId = -1;
    }
    // create Connection to the right switch
    // can potentially be a nullptr
    if (rightSwitchId != NULL_ID) {
        neighbors++;
        Switch::rightSwitchId = parseSwitchId(rightSwitchId);
        connections.emplace_back(Connection(Switch::switchId, Switch::rightSwitchId));
    } else {
        Switch::rightSwitchId = -1;
    }
    printf("I am switch: %i\n", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
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
    uint switchId = parseSwitchId(trafficFileItems.at(0));
    uint srcIP = static_cast<uint>(stoi(trafficFileItems.at(1)));
    uint dstIP = static_cast<uint>(stoi(trafficFileItems.at(2)));
    printf("Parsed trafficFileItem: switchId: %u srcIP: %u dst: %u\n",
           switchId, srcIP, dstIP);
    return make_tuple(switchId, srcIP, dstIP);
}

/**
 * Start the {@code Switch} loop.
 */
void Switch::start() {
    bool firstAck = true;
    char buf[1024];
    struct pollfd pfds[connections.size() + 1];

    // get fd for stdin
    pfds[0].fd = STDIN_FILENO;
    string line;
    ifstream trafficFileStream(trafficFile);

    for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
        printf("pfds[%lu] has connection: %s\n", i, connections[i - 1].getReceiveFIFOName().c_str());
        pfds[i].fd = connections[i - 1].openReceiveFIFO();
    }

    // When a switch starts, it sends an OPEN packet to the controller.
    // The carried message contains the switch number, the numbers of its neighbouring switches (if any),
    // and the range of IP addresses served by the switch.
    Message openMessage;
    openMessage.emplace_back(make_tuple("ID", to_string(switchId)));
    openMessage.emplace_back(make_tuple("N", to_string(neighbors)));
    openMessage.emplace_back(make_tuple("IPLow", to_string(ipLow)));
    openMessage.emplace_back(make_tuple("IPHigh", to_string(ipHigh)));
    Packet openPacket = Packet(OPEN, openMessage);
    write(connections[0].openSendFIFO(), openPacket.toString().c_str(), strlen(openPacket.toString().c_str()));
    tOpenCount++;
    // TODO: wait for ack?

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
                printf("read traffic file line: %s\n", line.c_str());
                if (line.length() < 1) {
                    printf("WARNING: ignoring invalid line: %s\n", line.c_str());
                } else if (line.substr(0, 1) == "#") {
                    printf("ignoring comment line\n");
                } else if (line.substr(0, 3) != "sw" + to_string(Switch::switchId)) {
                    printf("ignoring line specifying another switch\n");
                } else {
                    printf("found line specifying self: %s\n", line.c_str());
                    // TODO: refactor
                    trafficFileItem tfItem = parseTrafficItem(line);
                    uint tfSwitchId = get<0>(tfItem);
                    uint tfsrcIP = get<0>(tfItem);
                    uint tfdstIP = get<0>(tfItem);
                    // iterate through flowTable rules
                    for (auto const &flowEntry: flowTable) {
                        // TODO: check for matching rule for packet
                    }
                    // TODO: if no rule is found make query rule
                }
            } else {
                trafficFileStream.close();
                printf("finished reading traffic file\n");
            }
        }

        /*
         * 2. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        poll(pfds, connections.size() + 1, 0);
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
                printf("exit command received: terminating\n");
                exit(0);
            } else {
                printf("ERROR: invalid Controller command: %s\n"
                       "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
            }
        }

        /*
         * 3. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         *
         *    TODO: In addition,  upon receiving signal USER1, the switch displays the information specified by the list command
         */
        // iterate through each Connection (FIFO pair)
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

                Packet packet = Packet(cmd);
                string packetType = packet.getType();
                Message packetMessage = packet.getMessage();
                printf("Parsed packet: %s\n", packet.toString().c_str());
                if (firstAck && packetType == ACK) {
                    rAckCount++;
                    // do nothing on ack
                    firstAck = false;
                    printf("%s packet received: %s\n", packetType.c_str(), packet.toString().c_str());
                } else if (packetType == ADD) {
                    rAddCount++;

                    // ADD
                    // The switch then stores and
                    // applies the received rule
                    uint srcIP_lo = static_cast<uint>(stoi(get<1>(packetMessage[0])));
                    uint srcIP_hi = static_cast<uint>(stoi(get<1>(packetMessage[1])));
                    uint dstIP_lo = static_cast<uint>(stoi(get<1>(packetMessage[2])));
                    uint dstIP_hi = static_cast<uint>(stoi(get<1>(packetMessage[3])));
                    uint actionType = static_cast<uint>(stoi(get<1>(packetMessage[4])));
                    uint actionVal = static_cast<uint>(stoi(get<1>(packetMessage[5])));
                    uint pri = static_cast<uint>(stoi(get<1>(packetMessage[6])));
                    uint pktCount = static_cast<uint>(stoi(get<1>(packetMessage[7])));

                    printf("Parse %s packet: Adding new flowTable rule:\n"
                           "\tsrcIP_lo: %u srcIP_hi: %u dstIP_lo: %u dstIP_hi: %u actionType: %u actionVal: %u pri: %u pktCount: %u\n",
                           packetType.c_str(), srcIP_lo, srcIP_hi, dstIP_lo, dstIP_hi, actionType, actionVal, pri,
                           pktCount);

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
                } else if (packetType == RELAY) {
                    rRelayCount++;
                    // A switch may forward a received packet header to a neighbour (as instructed by a
                    // matching rule in the flow table).  This information is passed to the neighbour in a
                    // RELAY packet.
                    uint switchId = static_cast<uint>(stoi(get<1>(packetMessage[0])));
                    uint srcIp = static_cast<uint>(stoi(get<1>(packetMessage[1])));
                    uint dstIP = static_cast<uint>(stoi(get<1>(packetMessage[2])));

                    // iterate through flowTable rules
                    for (auto const &flowEntry: flowTable) {
                        // TODO: check for matching rule for packet
                    }
                    // TODO: if no rule is found make QUERY

//                    // TODO: use rules to figure out switch to write to
//                    uint targetSwitch; // TODO: INIT
//                    // TODO: parse RELAY packet
//                    Message relayMessage;
//                    relayMessage.emplace_back(make_tuple("ID", "sw" + to_string(switchId)));
//                    relayMessage.emplace_back(make_tuple("srcIP", srcIp));
//                    relayMessage.emplace_back(make_tuple("dstIP", dstIP));
//                    Packet relayPacket = Packet(RELAY, relayMessage);
//                    write(connections[targetSwitch].openSendFIFO(), relayPacket.toString().c_str(), strlen(relayPacket.toString().c_str()));
                        tRelayCount++;
                }  else {
                    // controller does nothing on ack, add, and relay
                    if (packet.getType() == ACK ) {
                        rAckCount++;
                    } else if(packet.getType() == OPEN){
                        rOpenCount++;
                    } else if (packet.getType() == RELAY) {
                        rQueryCount++;
                    }
                    printf("ERROR: unexpected %s packet received: %s\n", packet.getType().c_str(), cmd.c_str());
                }
            }
        }

    }
}

/**
 * Constructor by the controller to make a switch for upkeep.
 *
 * Note: this constructor not intended for actual usage by invoking the {@code start()} method.
 *
 * @param switchId
 * @param neighbors
 * @param ipLow
 * @param ipHigh
 */
Switch::Switch(uint switchId, uint neighbors, uint ipLow, uint ipHigh) : switchId(switchId), neighbors(neighbors),
                                                                         ipLow(ipLow), ipHigh(ipHigh) {

}

/**
 * Getter for a switch's {@code switchId}.
 *
 * @return {@code uint}
 */
uint Switch::getId() {
    return switchId;
}

/**
 * Getter for a switch's {@code ipHigh}.
 *
 * @return {@code uint}
 */
uint Switch::getIpHigh() {
    return ipHigh;
}

/**
 * Getter for a switch's {@code ipLow}.
 *
 * @return {@code uint}
 */
uint Switch::getIpLow() {
    return ipLow;
}

/**
 * Getter for a switch's {@code rightSwitchId}
 *
 * Note: if the switch does not have a right neighboring switch {@code -1} will be returned.
 *
 * @return {@code int}
 */
int Switch::getRightSwitchId() {
    return rightSwitchId;
}


/**
 * Getter for a switch's {@code leftSwitchId}
 *
 * Note: if the switch does not have a left neighboring switch {@code -1} will be returned.
 *
 * @return {@code int}
 */
int Switch::getLeftSwitchId() {
    return leftSwitchId;
}
