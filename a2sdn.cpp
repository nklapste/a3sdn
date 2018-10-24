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

#define MAX_NSW 7
#define MAXIP   1000

#define SWITCH_MODE "swi"
#define CONTROLLER_MODE "cont"

#define SWJ_FLAG "swj"
#define SWK_FLAG "swk"
#define NULL_STR "null"


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
    uint ipLow = stoul(ipRangeString.substr(0, ipRangeString.find(delimiter)));
    uint ipHigh = stoul(ipRangeString.substr(ipRangeString.find(delimiter) + 1, ipRangeString.size() - 1));
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


/**
 * Parse the swjFlag argument. Which follows the format [null|swj].
 *
 * @param swjFlag
 * @return {@code true} if "swj" is given, {@code false} if null is given, error and exit otherwise
 */
bool parseswjFlag(const string &swjFlag) {
    if (swjFlag == SWJ_FLAG) {
        return true;
    } else if (swjFlag == NULL_STR) {
        return false;
    } else {
        printf("ERROR: invalid swj Flag: %s\n"
               "\tFor swj flag: [null|swj]", swjFlag.c_str());
        exit(1);
    }
}


/**
 * Parse the swkFlag argument. Which follows the format [null|swk].
 *
 * @param swkFlag
 * @return {@code true} if "swk" is given, {@code false} if null is given, error and exit otherwise
 */
bool parseswkFlag(const string &swkFlag) {
    if (swkFlag == SWK_FLAG) {
        return true;
    } else if (swkFlag == NULL_STR) {
        return false;
    } else {
        printf("ERROR: invalid swk Flag: %s\n"
               "\tFor swk flag: [null|swk]", swkFlag.c_str());
        exit(1);
    }
}

////////////////////////
// Main entrypoint
////////////////////////

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
               "\tOr follow switch mode:         'a2sdn swi trafficFile [null|swj] [null|swk] ipLow-IPhig'");
        exit(1);
    }
    string mode = argv[1];

    if (mode == CONTROLLER_MODE) {
        // parse controller mode arguments
        if (argc != 3) {
            printf("ERROR: invalid arguments for controller mode:\n"
                   "\tFor controller mode: 'a2sdn cont nSwitch'");
            exit(1);
        }
        uint nSwitch = stoul(argv[2]);
        // TODO:
    } else if (mode == SWITCH_MODE) {
        // parse switch mode arguments
        if (argc != 6) {
            printf("ERROR: invalid arguments for switch mode:\n"
                   "\tFor switch mode: 'a2sdn swi trafficFile [null|swj] [null|swk] ipLow-IPhig'");
            exit(1);
        }
        string trafficFile = argv[2];
        bool swjFlag = parseswjFlag(argv[3]);
        bool swkFlag = parseswkFlag(argv[4]);
        ipRange ipRange1 = parseIPRange(argv[5]);
        // TODO:
        Controller(get<0>(ipRange1), get<1>(ipRange1));

    } else {
        printf("ERROR: invalid mode specified: %s", argv[2]);
        exit(1);
    }
    return 0;
}
