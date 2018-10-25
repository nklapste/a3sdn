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
#include "controller.h"

using namespace std;

/**
 * Parse the switch id. Should match the format {@code 'swi'} where {@code 'i'} is a numeric.
 * @param switchId
 * @return
 */
uint parseSwitchId(const string &switchId) {
    regex rgx("(sw)([0-9]+)");
    match_results<string::const_iterator > matches;

    std::regex_match(switchId, matches, rgx);
    for( std::size_t index = 1; index < matches.size(); ++index ){
    }

    if(std::regex_search(switchId, matches, rgx)) {
        return static_cast<uint>(std::stoi(matches[2], nullptr, 10));
    } else {
        // we failed to parse the switch id
        printf("ERROR: invalid switch id argument: %s", switchId.c_str());
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
Switch::Switch(string &switchId, string &leftSwitchId, string &rightSwitchId, string &trafficFile, uint ipLow, uint ipHigh) {
    printf("Creating switch: %s trafficFile: %s swj: %s swk: %s ipLow: %u ipHigh: %u",
           switchId.c_str(), trafficFile.c_str(), leftSwitchId.c_str(), rightSwitchId.c_str(),ipLow, ipHigh);
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

    flowTable.push_back(init_rule);

    Switch::trafficFile = trafficFile;

    // create Connection to controller
    Switch::switchId = parseSwitchId(switchId);
    connections.emplace_back(Connection(Switch::switchId, CONTROLLER_ID));

    // create Connection to the left switch
    // can potentially be a nullptr
    if(leftSwitchId != NULL_ID){
        Switch::leftSwitchId = parseSwitchId(leftSwitchId);
        connections.emplace_back(Connection(Switch::switchId, CONTROLLER_ID));
    } else {
        // TODO:
        connections.emplace_back(Connection(Switch::switchId, CONTROLLER_ID));
    }

    // create Connection to the right switch
    // can potentially be a nullptr
    if(rightSwitchId != NULL_ID){
        Switch::rightSwitchId = parseSwitchId(rightSwitchId);
        connections.emplace_back(Connection(Switch::switchId, CONTROLLER_ID));
    } else {
        // TODO:
        connections.emplace_back(Connection(Switch::switchId, CONTROLLER_ID));
    }
    printf("I am switch: %i\n", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}

/**
 * Getter for the connection to the left neighboring switch.
 * @return
 */
Connection Switch::getRightSwitchConnection() {
    return connections.at(2);
}

/**
 * Getter for the connection to the right neighboring switch.
 * @return
 */
Connection Switch::getLeftSwitchConnection() {
    return connections.at(1);
}

/**
 * Getter for the connection to the controller.
 * @return
 */
Connection Switch::getControllerConnection() {
    return connections.at(0);
}

/**
 * Start the {@code Switch} loop.
 */
void Switch::start() {
    for(;;){
       /*
        * 1.  Read and process a single line from the traffic line (if the EOF has not been reached yet). The
        *     switch ignores empty lines, comment lines, and lines specifying other handling switches. A
        *     packet header is considered admitted if the line specifies the current switch
        *
        *     Note: After reading all lines in the traffic file, the program continues to monitor and process
        *     keyboard commands, and the incoming packets from neighbouring devices.
        */
        string line;
        ifstream trafficFileStream(trafficFile);
        for (int lineNo = 0; getline(trafficFileStream, line) && !trafficFileStream.eof(); lineNo++) {
            // TODO: implement
        }

        /*
         * 2. Poll the keyboard for a user command. The user can issue one of the following commands.
         *       list: The program writes all entries in the flow table, and for each transmitted or received
         *             packet type, the program writes an aggregate count of handled packets of this
         *             type.
         *       exit: The program writes the above information and exits.
         */
        // TODO: implement

        /*
         * 3. Poll the incoming FIFOs from the controller and the attached switches. The switch handles
         *    each incoming packet, as described in the Packet Types.
         *
         *    In addition,  upon receiving signal USER1, the switch displays the information specified by the list command
         */
        // TODO: implement
    }
}

