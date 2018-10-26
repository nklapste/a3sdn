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
#include "controller.h"
#include "packet.h"

#define STDIN_INDEX 0
#define CONTROLLER_INDEX 1
#define LEFT_SWITCH_INDEX 2
#define RIGHT_SWITCH_INDEX 3

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
    flowEntry init_rule = {
            .srcIP_lo = 0,
            .srcIP_hi = MAX_IP,
            .dstIP_lo = ipLow,
            .dstIP_hi = ipHigh,
            .actionType= FORWARD,  // TODO: help ambiguous
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
    }
    // create Connection to the right switch
    // can potentially be a nullptr
    if (rightSwitchId != NULL_ID) {
        neighbors++;
        Switch::rightSwitchId = parseSwitchId(rightSwitchId);
        connections.emplace_back(Connection(Switch::switchId, Switch::rightSwitchId));
    }
    printf("I am switch: %i\n", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}

/**
 * Start the {@code Switch} loop.
 */
void Switch::start() {
    char buf[1024];
    struct pollfd pfds[connections.size() + 1];

    // get fd for stdin
    pfds[0].fd = STDIN_FILENO;
    bool firstAck = true;
    string line;
    ifstream trafficFileStream(trafficFile);

    for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
        printf("pfds[%lu] has connection: %s\n", 2 * i, connections[i - 1].getSendFIFOName().c_str());
        pfds[2 * i].fd = connections[i - 1].openSendFIFO();
        printf("pfds[%lu] has connection: %s\n", 2 * i - 1, connections[i - 1].getReceiveFIFOName().c_str());
        pfds[2 * i - 1].fd = connections[i - 1].openReceiveFIFO();
    }

    // send a OPEN packer to the controller
    string openPacketRaw =
            "OPEN: ID:" + std::to_string(switchId) + " N:" + to_string(neighbors) + " IPLow:" + to_string(ipLow) +
            " IPHigh:" + to_string(ipHigh);
    Packet openPacket = Packet(openPacketRaw);
    write(connections[0].openSendFIFO(), openPacket.toString().c_str(), strlen(openPacket.toString().c_str()));
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

//        pfds[0].events = POLLIN;
//        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
//            pfds[i].events = POLLIN;
//        }

        poll(pfds, connections.size() + 1, 0);
//        if (errno || p==-1){
//            perror("ERROR: calling poll");
//        }
        if (pfds[0].revents & POLLIN) {
            int r = read(pfds[0].fd, buf, 1024);
            if (!r) {
                printf("WARNING: stdin closed\n");
            }
            string cmd = string(buf);
            // trim off all whitespace
            while (!cmd.empty() && !std::isalpha(cmd.back())) cmd.pop_back();
            if (cmd == LIST_CMD) {
                // TODO: implement
                write(connections[0].openSendFIFO(), cmd.c_str(), strlen(cmd.c_str()));
                write(STDOUT_FILENO, buf, r);
            } else if (cmd == EXIT_CMD) {
                // TODO: write above information
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
         *    In addition,  upon receiving signal USER1, the switch displays the information specified by the list command
         */
        // iterate through each Connection (FIFO pair)
        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
            if (pfds[2 * i - 1].revents & POLLIN) {
                printf("pfds[%lu] has connection POLLIN event: %s\n", 2 * i - 1,
                       connections[i - 1].getReceiveFIFOName().c_str());
                int r = read(pfds[2 * i - 1].fd, buf, 1024);
                if (!r) {
                    printf("WARNING: receiveFIFO closed\n");
                }
                string cmd = string(buf);
                printf("Received output: %s\n", cmd.c_str());

                // TODO: handle packets
                Packet packet = Packet(cmd);
                string packetType = packet.getType();
                Message packetMessage = packet.getMessage();
                printf("Parsed packet: %s\n", packet.toString().c_str());
                if (firstAck && packetType == ACK) {
                    // do nothing on ack
                    firstAck = false;
                    printf("%s packet received: %s\n", packetType.c_str(), packet.toString().c_str());
                } else if (packetType == ADD) {
                    //ADD
                    // The switch then stores and
                    // applies the received rule
                    // TODO: parse ADD packet
                    //flowTable.emplace_back();
                } else if (packetType == RELAY) {
                    // A switch may forward a received packet header to a neighbour (as instructed by a
                    // matching rule in the flow table).  This information is passed to the neighbour in a
                    // RELAY packet.
                    // TODO: parse RELAY packet
//                         string relayPacket = "OPEN: ID:"+std::to_string(switchId)+" N:"+to_string(neighbors)+" IP:"+to_string(ipLow)+"-"+to_string(ipHigh);
//                         write(connections[0].openSendFIFO(), openPacket.c_str(), strlen(openPacket.c_str()));
                } else if (packetType == OPEN || packetType == QUERY) {
                    // switch does not handle open, or query
                    printf("ERROR: unexpected %s packet received: %s", packetType.c_str(),
                           packet.toString().c_str());
                } else {
                    printf("ERROR: unknown packet received: %s\n", cmd.c_str());
                }
            }
        }

    }
}

