/**
 * a2sdn controller.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include "controller.h"
#include <sys/types.h>
#include <tuple>
#include <iostream>
#include <fstream>

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


using namespace std;

/**
 * Initialize a Controller.
 *
 * @param nSwitches
 */
Controller::Controller(uint nSwitches): nSwitches(nSwitches)  {
    if (nSwitches>MAX_SWITCHES){
        printf("ERROR: too many switches for controller: %u\n"
               "\tMAX_SWITCHES=7\n", nSwitches);
        exit(1);
    }
    // TODO: this is not printing well
    printf("Creating controller: nSwitches: %u\n", nSwitches);
    // init all potential switch connections for the controller
    for (uint switch_i=1; switch_i<=nSwitches; ++switch_i) {
        connections.emplace_back(CONTROLLER_ID, switch_i);
    }
}

/**
 * Returns the number of switches bound to the controller.
 *
 * @return
 */
uint Controller::getNumSwitches() {
    /* Returns the number of switches */
    return nSwitches;
}

/**
 * Get the connection to a specified switch.
 *
 * @param switchId the numeric id of the switch.
 * @return
 */
Connection Controller::getConnection(uint switchId){
    return Controller::connections.at(switchId-1);
}

/**
 * Start the {@code Controller} loop.
 */
void Controller::start() {
    struct pollfd pfds[getNumSwitches()+2];
    char buf[1024];

    // get fd for stdin
    pfds[0].fd = 0;
    pfds[0].events = POLLIN;

    // get fd for receive FIFOs
    for(uint switchId = 1; switchId != getNumSwitches()+1; switchId++) {
        pfds[switchId].fd = getConnection(switchId).getReceiveFIFO();
    }

    for(;;){
        /*
         * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        pfds[0].events = POLLIN;
        poll(pfds, 0, 0);
        if(pfds[0].revents & POLLIN) {
            int i = read(pfds[0].fd, buf, 1024);
            if (!i) {
                printf("stdin closed\n");
            }
            string cmd = string(buf);
            // trim off all whitespace
            while(!cmd.empty() && !std::isalpha(cmd.back())) cmd.pop_back() ;

            if (cmd == LIST_CMD) {
                // TODO: implement
                write(getConnection(1).getSendFIFO(), "123", sizeof("123"));
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
         * 2. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         *
         *    In addition,  upon receiving signal USER1, the switch displays the information specified by the
         *    list command
         */
        for(vector<Connection>::size_type switchId = 1; switchId < getNumSwitches()+1; switchId++) {
            pfds[switchId].events = POLLIN;
            poll(pfds, switchId, 0);
            if (pfds[switchId].revents & POLLIN) {
                int i = read(pfds[switchId].fd, buf, 1024);
                if (!i) {
                    printf("receive FIFO closed\n");
                }
                string message = string(buf);
                printf("got message: %s", message.c_str());
            }
            close(pfds[switchId].fd);
        }
    }
}
