/**
 * a2sdn
 *
 * Mainly contains argparsing functionality and the initialization of
 * either the {@code Controller} or {@code Switch}.
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include "controller.h"
#include "switch.h"

#define MAXIP   1000
#define CONTROLLER_MODE "cont"

using namespace std;

////////////////////////////////
// Argument Parsing functions
////////////////////////////////

/**
 * Parse the ip range argument. Which follows the format ipLow-ipHigh.
 *
 * @param ipRangeString
 * @return {@code ipRange} a tuple of (ipLow, ipHigh)
 */
ipRange parseIPRange(const string &ipRangeString) {
    string delimiter = "-";
    uint ipLow = static_cast<uint>(stoi(ipRangeString.substr(0, ipRangeString.find(delimiter))));
    uint ipHigh = static_cast<uint>(stoi(ipRangeString.substr(ipRangeString.find(delimiter) + 1, ipRangeString.size() - 1)));
    if (ipLow > ipHigh) {
        printf("ERROR: invalid ip range:\n"
               "\tipLow: %u greater than ipHigh: %u", ipLow, ipHigh);
        exit(1);
    }
    if (ipHigh - ipLow > MAXIP) {
        printf("ERROR: invalid ip range:\n"
               "\tip range is too large: %u", ipHigh - ipLow);
        exit(1);
    }
    return make_tuple(ipLow, ipHigh);
}


////////////////////////
// Main entrypoint
////////////////////////

/**
 * Parse the command line arguements from main().
 *
 * @param argc
 * @param argv
 * @return
 */
void parseArgs(int argc, char **argv){

}


/**
 * Main entry point for a2sdn.
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {
    if (argc < 3 || argc > 6) {
        printf("ERROR: invalid argument format:\n"
               "\tPlease follow controller mode: 'a2sdn cont nSwitch'\n"
               "\tOr follow switch mode:         'a2sdn swi trafficFile [null|swj] [null|swk] ipLow-IPhigh'\n");
        exit(1);
    }
    string mode = argv[1];

    if (mode == CONTROLLER_MODE) {
        // parse controller mode arguments
        if (argc != 3) {
            printf("ERROR: invalid arguments for controller mode:\n"
                   "\tFor controller mode: 'a2sdn cont nSwitch'\n");
            exit(1);
        }
        Controller controller = Controller((uint) stoi(argv[2]));
        controller.start();
    } else {
        // parse switch mode arguments
        if (argc != 6) {
            printf("ERROR: invalid arguments for switch mode:\n"
                   "\tFor switch mode: 'a2sdn swi trafficFile [null|swj] [null|swk] ipLow-IPhig'\n");
            exit(1);
        }
        string switchId = argv[1];
        string trafficFile = argv[2];
        string leftSwitchId = argv[3];
        string rightSwitchId = argv[4];
        ipRange ipRange1 = parseIPRange(argv[5]);
        Switch aSwitch = Switch(switchId, leftSwitchId, rightSwitchId, trafficFile, get<0>(ipRange1), get<1>(ipRange1));
        aSwitch.start();
    }
    return 0;
}
