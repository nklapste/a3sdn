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
 * Start the {@code Controller} loop.
 */
void Controller::start() {
    struct pollfd pfds[connections.size()+1];
    char buf[1024];

    // get fd for stdin
    pfds[0].fd = STDIN_FILENO;

    // get fd for receive FIFOs
    for(std::vector<Connection>::size_type i = 0; i != connections.size(); i++) {
        pfds[1 + i].fd = connections[i].getReceiveFIFO();
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
                write(connections[0].getSendFIFO(), "123", sizeof("123"));
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
        for(std::vector<Connection>::size_type i = 0; i != connections.size(); i++) {
            pfds[i + 1].events = POLLIN;
            poll(pfds, i + 1, 0);
            if(pfds[i + 1].revents & POLLIN) {
                int r = read(pfds[i + 1].fd, buf, 1024);
                if (!r) {
                    printf("stdin closed\n");
                }
                string cmd = string(buf);
                printf("Received message: %s", cmd.c_str());
            }
        }
    }
}
