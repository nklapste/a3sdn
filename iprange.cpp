//
// Created by nklap on 2018-11-09.
//

#include "iprange.h"
#include "flow.h"

/**
 * Parse the {@code IPRange} argument. Which follows the format IPLow-IPHigh.
 *
 * @param IPRangeString {@code std::string}
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
