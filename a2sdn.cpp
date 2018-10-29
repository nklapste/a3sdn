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
#include "controller.h"

#define CONTROLLER_MODE "cont"

using namespace std;

////////////////////////////////
// Argument Parsing functions
////////////////////////////////

/**
 * Parse the IP range argument. Which follows the format IPLow-IPHigh.
 *
 * @param IPRangeString
 * @return {@code IPRange} a tuple of (IPLow, IPHigh)
 */
IPRange parseIPRange(const string &IPRangeString) {
    string delimiter = "-";
    uint IPLow = static_cast<uint>(stoi(IPRangeString.substr(0, IPRangeString.find(delimiter))));
    uint IPHigh = static_cast<uint>(stoi(
            IPRangeString.substr(IPRangeString.find(delimiter) + 1, IPRangeString.size() - 1)));
    if (IPHigh > MAX_IP) {
        printf("ERROR: invalid IPHigh: %u MAX_IP: %u", IPHigh, MAX_IP);
        exit(EINVAL);
    }
    if (IPLow < MIN_IP) {
        printf("ERROR: invalid IPLow: %u MIN_IP: %u", IPLow, MIN_IP);
        exit(EINVAL);
    }
    if (IPLow > IPHigh) {
        printf("ERROR: invalid IP range: IPLow: %u greater than IPHigh: %u", IPLow, IPHigh);
        exit(EINVAL);
    }
    return make_tuple(IPLow, IPHigh);
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
               "\tOr follow switch mode:         'a2sdn swi trafficFile [null|swj] [null|swk] IPLow-IPhigh'\n");
        exit(EINVAL);
    }
    string mode = argv[1];

    if (mode == CONTROLLER_MODE) {
        // parse controller mode arguments
        if (argc != 3) {
            printf("ERROR: invalid arguments for controller mode:\n"
                   "\tFor controller mode: 'a2sdn cont nSwitch'\n");
            exit(EINVAL);
        }
        Controller controller = Controller((uint) stoi(argv[2]));
        controller.start();
    } else {
        // parse switch mode arguments
        if (argc != 6) {
            printf("ERROR: invalid arguments for switch mode:\n"
                   "\tFor switch mode: 'a2sdn swi trafficFile [null|swj] [null|swk] IPLow-IPHigh'\n");
            exit(EINVAL);
        }
        string switchId = argv[1];
        string trafficFile = argv[2];
        string leftSwitchId = argv[3];
        string rightSwitchId = argv[4];
        IPRange IPRange1 = parseIPRange(argv[5]);
        Switch aSwitch = Switch(switchId, leftSwitchId, rightSwitchId, trafficFile, get<0>(IPRange1), get<1>(IPRange1));
        aSwitch.start();
    }
    return 0;
}
