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

#include <sys/signalfd.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <string.h>

#include <assert.h>
using namespace std;




void handle_signal(int signo);


void handle_signal(int signo)
{
    if( signo == SIGUSR1 )
    {
        write( 1, "Received user signal\n", 21);
    }
    else
    {
        write( 1, "unexpected signal received\n", 27 );
    }
}
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
        printf("[sw%u] port1= %i, port2= %i, port3= %u-%u\n",
               sw.getID(), sw.getLeftSwitchID(), sw.getRightSwitchID(), sw.getIPLow(), sw.getIPHigh());
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
    struct pollfd pfds[connections.size() + 3];
    char buf[1024];

    // setup file descriptions or stdin and all connection FIFOs
    pfds[0].fd = STDIN_FILENO;
    for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
        pfds[i].fd = connections[i - 1].openReceiveFIFO();
    }

    int err;
    sigset_t sigset;
    /* Create a sigset of all the signals that we're interested in */
    err = sigemptyset(&sigset);
    assert(err == 0);
    err = sigaddset(&sigset, SIGUSR1);
    assert(err == 0);
    /* We must block the signals in order for signalfd to receive them */
    err = sigprocmask(SIG_BLOCK, &sigset, NULL);
    assert(err == 0);
    /* This is the main loop */
    pfds[connections.size()+2].fd = signalfd(-1, &sigset, 0);;
    pfds[connections.size()+2].events = POLLIN;

    for (;;) {
        /*
         * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
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

        int ret = poll(pfds, connections.size() + 3, 0);
        if (errno || ret<0){
            perror("poll failure");
        }
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
            fflush(stdout);
            fflush(stdin);
        }

        /*
         * 2. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         */
        for (std::vector<Connection>::size_type i = 1; i != connections.size() + 1; i++) {
            if (pfds[i].revents & POLLIN) {
                printf("pfds[%lu] has connection POLLIN event: %s\n", i,
                       connections[i - 1].getReceiveFIFOName().c_str());
                memset(buf, 0, sizeof buf);
                int r = read(pfds[i].fd, buf, 1024);
                if (!r) {
                    printf("WARNING: receiveFIFO closed\n");
                }
                string cmd = string(buf);

                // take the message and parse it into a packet
                Packet packet = Packet(cmd);
                printf("Parsed packet: %s\n", packet.toString().c_str());

                if (packet.getType() == OPEN) {
                    openResponse(connections[i - 1], packet.getMessage());
                } else if (packet.getType() == QUERY) {
                    queryResponse(connections[i - 1], packet.getMessage());
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

        // TODO:
        /*
         * In addition, upon receiving signal USER1, the switch displays the information specified by the list command.
         */
        if(pfds[connections.size()+2].revents & POLLIN){
            // TODO works but blocks stuff
            struct signalfd_siginfo info;
            /* We have a valid signal, read the info from the fd */
            int r = read(pfds[connections.size()+2].fd, &info, sizeof(info));
            unsigned sig = info.ssi_signo;
            if (sig == SIGUSR1) {
                printf("received SIGUSR1\n");
                list();
            }
        }
    }
}


/**
 * Calculate a new flow entry rule.
 *
 * @param switchID
 * @param srcIP
 * @param dstIP
 * @return
 */
FlowEntry Controller::makeRule(uint switchID, uint srcIP, uint dstIP) {
    auto it = find_if(switches.begin(), switches.end(), [&switchID](Switch &sw) { return sw.getID() == switchID; });
    if (it != switches.end()) {
        // found element. it is an iterator to the first matching element.
        auto index = std::distance(switches.begin(), it);
        Switch requestSwitch = switches[index];
        // check src IP is invalid
        if (srcIP < 0 || srcIP > MAX_IP) {
            // src IP is invalid make a drop rule
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
            if (dstIP < requestSwitch.getIPLow() || dstIP > requestSwitch.getIPHigh()) { // out of range of
                // get left switch
                int leftSwitchID = requestSwitch.getLeftSwitchID();
                if (leftSwitchID > 0) {
                    auto it2 = find_if(switches.begin(), switches.end(),
                                       [&leftSwitchID](Switch &sw) { return sw.getID() == leftSwitchID; });
                    if (it != switches.end()) {
                        auto index2 = std::distance(switches.begin(), it2);
                        Switch requestLeftSwitch = switches[index2];
                        if (dstIP < requestLeftSwitch.getIPLow() || dstIP > requestLeftSwitch.getIPHigh()) {
                        } else {
                            FlowEntry forwardLeftRule = {
                                    .srcIP_lo   = 0,
                                    .srcIP_hi   = MAX_IP,
                                    .dstIP_lo   = dstIP,
                                    .dstIP_hi   = dstIP,
                                    .actionType = FORWARD,
                                    .actionVal  = requestLeftSwitch.getID(),
                                    .pri        = MIN_PRI,
                                    .pktCount   = 0
                            };
                            return forwardLeftRule;
                        }
                    }
                }
                // get the right switch
                int rightSwitch = requestSwitch.getRightSwitchID();
                if (rightSwitch > 0) {
                    auto it3 = find_if(switches.begin(), switches.end(),
                                       [&rightSwitch](Switch &sw) { return sw.getID() == rightSwitch; });
                    if (it != switches.end()) {
                        auto index3 = std::distance(switches.begin(), it3);
                        Switch requestRightSwitch = switches[index3];
                        if (dstIP < requestRightSwitch.getIPLow() || dstIP > requestRightSwitch.getIPHigh()) {
                        } else {
                            FlowEntry forwardRightRule = {
                                    .srcIP_lo   = 0,
                                    .srcIP_hi   = MAX_IP,
                                    .dstIP_lo   = dstIP,
                                    .dstIP_hi   = dstIP,
                                    .actionType = FORWARD,
                                    .actionVal  = requestRightSwitch.getID(),
                                    .pri        = MIN_PRI,
                                    .pktCount   = 0
                            };
                            return forwardRightRule;
                        }
                    }
                }
                // all other options exhausted
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
        printf("ERROR: attempted to make rule for switch that is not supported: switchID: %u", switchID);
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


void Controller::queryResponse(Connection connection, Message message) {
    rQueryCount++;
    // When processing an incoming packet header (the header may be read from
    //the traffic file, or relayed to the switch by one of its neighbours), if a switch does not find
    //a matching rule in the flow table, the switch sends a
    //QUERY
    //packet to the controller.  The
    //controller replies with a rule stored in a packet of type
    uint switchID = static_cast<uint>(stoi(get<1>(message[0])));
    uint srcIP = static_cast<uint>(stoi(get<1>(message[1])));
    uint dstIP = static_cast<uint>(stoi(get<1>(message[2])));
    printf("Parsed QUERY packet: switchID: %u srcIP: %u dstIP: %u\n",
           switchID, srcIP, dstIP);

    // calculate new flow entry
    FlowEntry flowEntry = makeRule(switchID, srcIP, dstIP);

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
    write(connection.openSendFIFO(), addPacket.toString().c_str(),
          strlen(addPacket.toString().c_str()));
    tAddCount++;
}

void Controller::openResponse(Connection connection, Message message) {
    rOpenCount++;
    // Upon receiving an OPEN packet, the controller updates its stored information about the switch,
    // and replies with a packet of type ACK
    uint switchID = static_cast<uint>(stoi(get<1>(message[0])));
    int leftSwitchID = stoi(get<1>(message[1]));
    int rightSwitchID = stoi(get<1>(message[2]));
    uint switchIPLow = static_cast<uint>(stoi(get<1>(message[3])));
    uint switchIPHigh = static_cast<uint>(stoi(get<1>(message[4])));
    printf("Parsed OPEN packet: switchID: %u leftSwitchID: %i rightSwitchID: %i switchIPLow: %u switchIPHigh: %u\n",
           switchID, leftSwitchID, rightSwitchID, switchIPLow, switchIPHigh);

    switches.emplace_back(
            Switch(switchID, leftSwitchID, rightSwitchID, switchIPLow, switchIPHigh));

    // send ack back to switch
    printf("Sending ACK back to switchID: %u\n", switchID);
    Packet ackPacket = Packet(ACK, Message());
    write(connection.openSendFIFO(), ackPacket.toString().c_str(),
          strlen(ackPacket.toString().c_str()));
    tAckCount++;
}
