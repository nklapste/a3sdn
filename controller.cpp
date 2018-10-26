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
    printf("Created controller\n");
}

/**
 * Start the {@code Controller} loop.
 */
void Controller::start() {
    struct pollfd pfds[2*connections.size()+1];
    char buf[1024];

    // get fd for stdin
    pfds[0].fd = STDIN_FILENO;

    // TODO: this is blocking
    // TODO listing the fifo

    for(std::vector<Connection>::size_type i = 1; i != connections.size()+1; i++) {
        printf("pfds[%lu] has connection: %s\n", 2*i, connections[i-1].getSendFIFOName().c_str());
        pfds[2*i].fd = connections[i-1].openSendFIFO();
        printf("pfds[%lu] has connection: %s\n", 2*i-1, connections[i-1].getReceiveFIFOName().c_str());
        pfds[2*i-1].fd = connections[i-1].openReceiveFIFO();
    }

    for(;;){

        /*
         * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
//        pfds[0].events = POLLIN;
//        for(std::vector<Connection>::size_type i = 1; i != connections.size()+1; i++) {
//            pfds[i].events = POLLIN;
//        }

        int p = poll(pfds, 2*connections.size()+1, 0);
//        if (errno || p==-1){
//            perror("ERROR: calling poll");
//        }
        if(pfds[0].revents & POLLIN) {
            int r = read(pfds[0].fd, buf, 1024);
            if (!r) {
                printf("stdin closed\n");
            }
            string cmd = string(buf);
            // trim off all whitespace
            while(!cmd.empty() && !std::isalpha(cmd.back())) cmd.pop_back() ;

            if (cmd == LIST_CMD) {
                // TODO: implement
                write(connections[0].openSendFIFO(), cmd.c_str(), strlen(cmd.c_str()));
                write(STDOUT_FILENO, cmd.c_str(), strlen(cmd.c_str()));
            } else if (cmd == EXIT_CMD) {
                // TODO: write above information
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
        for(std::vector<Connection>::size_type i = 1; i != connections.size()+1; i++) {
            if(pfds[2*i-1].revents & POLLIN) {
                printf("pfds[%lu] has connection POLLIN event: %s\n", 2*i-1, connections[i-1].getReceiveFIFOName().c_str());
                int r = read(pfds[2*i-1].fd, buf, 1024);
                if (!r) {
                    printf("stdin closed\n");
                }
                string cmd = string(buf);
                printf("Received output: %s\n", cmd.c_str());
            }
        }
    }
}
