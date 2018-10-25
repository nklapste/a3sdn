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
               "\tMAX_SWITCHES=7", nSwitches);
        exit(1);
    }
    printf("Creating controller: nSwitches: %u", nSwitches);
    // init all potential switch connections for the controller
    for (uint switch_i=0; switch_i<nSwitches; ++switch_i) {
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
 * @param switchId
 * @return
 */
Connection Controller::getConnection(uint switchId){
    return Controller::connections.at(switchId);
}

/**
 * Start the {@code Controller} loop.
 */
void Controller::start() {
    char buf[1024];
    for(;;){
        /*
         * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        struct pollfd fds;
        int ret;
        fds.fd = 0; /* this is STDIN */
        fds.events = POLLIN;
        ret = poll(&fds, 1, 0);
        if(ret == 1) {
            read(0, buf, 1024);
            string cmd = string(buf);
            // trim off all whitespace
            while( !cmd.empty() && !std::isalpha( cmd.back() ) ) cmd.pop_back() ;
            if (cmd == LIST_CMD) {
                // TODO: implement
            } else if (cmd == EXIT_CMD) {
                // TODO: write above information
                exit(0);
            } else {
                printf("ERROR: invalid Controller command: %s\n"
                       "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
            }
            memset(buf, 0, sizeof buf);
        } else if(ret != 0) {
            perror("ERROR: polling stdin");
        }

        /*
         * 2. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         *
         *    In addition,  upon receiving signal USER1, the switch displays the information specified by the
         *    list command
         */


    }
}
