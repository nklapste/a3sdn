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

#define LIST_CMD "list"
#define EXIT_CMD "exit"
#define BOOL_STR(b) (b?"true":"false")


using namespace std;


/**
 * Startup a2sdn in controller mode.
 *
 * @param trafficFile
 * @param swjFlag
 * @param swkFlag
 * @param ipRange1
 */
void controllerMode(const string &trafficFile, bool swjFlag, bool swkFlag, ipRange ipRange1) {
    printf("Starting switch mode: trafficFile: %s swjFlag: %s swkFlag: %s ipLow: %u ipHigh: %u",
           trafficFile.c_str(), BOOL_STR(swjFlag), BOOL_STR(swkFlag), get<0>(ipRange1), get<1>(ipRange1));
    uint i = 1;
    for (;;) {
        /*1. Read and process a single line from the traffic line (if the EOF has not been reached yet). The
        switch ignores empty lines, comment lines, and lines specifying other handling switches. A
        packet header is considered admitted if the line specifies the current switch.
        */
        string line;
        ifstream trafficFileStream(trafficFile);
        for (int lineNo = 0; getline(trafficFileStream, line) && !trafficFileStream.eof(); lineNo++) {
            // TODO: implement
        }

        /*
        2. Poll the keyboard for a user command. The user can issue one of the following commands.
        • list: The program writes all entries in the flow table, and for each transmitted or received
        packet type, the program writes an aggregate count of handled packets of this
        type.
        • exit: The program writes the above information and exits.
        */
        if (i == 1) exit(0);

        // TODO: implement properly
        string cmd;
        if (cmd == LIST_CMD) {
            // TODO: implement
        } else if (cmd == EXIT_CMD) {
            // TODO: write above information
            exit(0);
        } else {
            printf("ERROR: invalid controllerMode commmand: %s\n"
                   "\tPlease use either 'list' or 'exit'", cmd.c_str());
            exit(1);
        }
    }
}

#include "controller.h"

/*FIFO stuff*/
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>


Controller::Controller(int num, int max): nSwitches(num)  {
    name = "fifo-0-0";
}

int Controller::getNumSwitches() {
    /* Returns the number of switches */
    return nSwitches;
}

void Controller::makeFIFO(string &trafficFile) {
    /* Make the FIFO */
    int status = mkfifo(trafficFile.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                            S_IWGRP | S_IROTH | S_IWOTH);
    if (errno){
        // TODO: print error
    }
}


void Controller::initConn(int id) {
}

void Controller::openConn(char id) {
}

int Controller::openReadFIFO(int id) {
    /* Opens a FIFO for reading a switch with id. */
    makeFIFO(getFIFOName(id));
    return open(getFIFOName(id), O_RDONLY);
}

int Controller::openWriteFIFO(int id) {
    /* Opens a FIFO for writing a switch with id. */
    makeFIFO(getFIFOName(id));
    return open(getFIFOName(id), O_WRONLY);
}

void Controller::addFIFO(int id) {
    /* Add FIFOs for reading and writing for a switch to list of FIFOs. */
    openReadFIFO(id);
    openWriteFIFO(id);
    // Add the connection in the connections array.
    return open(getFIFOName(id), O_RDONLY);
}


const string Controller::getFIFOName(int x, int y) {
    return "fifo-" + std::to_string(x) + "-" + std::to_string(y);
}